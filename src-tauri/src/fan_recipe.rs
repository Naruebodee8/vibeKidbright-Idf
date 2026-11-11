//! Checked classic KidBright fan recipe. Firmware and simulator share one port plan.
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};

#[derive(Clone, Copy, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum FanPort {
    Usb,
    Out1,
    Out2,
}
impl FanPort {
    pub fn gpio(self) -> u8 {
        match self {
            Self::Usb => 25,
            Self::Out1 => 26,
            Self::Out2 => 27,
        }
    }
    pub fn key(self) -> &'static str {
        match self {
            Self::Usb => "usb",
            Self::Out1 => "out1",
            Self::Out2 => "out2",
        }
    }
    pub fn api(self) -> &'static str {
        match self {
            Self::Usb => "usbsw_write",
            Self::Out1 => "output1_write",
            Self::Out2 => "output2_write",
        }
    }
    pub fn on_level(self) -> u8 {
        if self == Self::Usb {
            0
        } else {
            1
        }
    }
}

pub fn firmware(port: FanPort) -> String {
    format!(
        r#"#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Classic KidBright reference wiring. Verify the physical board revision.
// USB_SW GPIO25: active LOW per the classic board reference; OUT gates: HIGH sinks.
#define SW1_GPIO GPIO_NUM_16
#define FAN_GPIO GPIO_NUM_{gpio}
#define FAN_ON_LEVEL {on}
#define FAN_OFF_LEVEL {off}
#define SAMPLE_MS 10
#define STABLE_SAMPLES 3
static const char *TAG = "FAN_CONTROL";

static void fan_write(bool on) {{
    ESP_ERROR_CHECK(gpio_set_level(FAN_GPIO, on ? FAN_ON_LEVEL : FAN_OFF_LEVEL));
}}

void app_main(void) {{
    gpio_config_t button = {{
        .pin_bit_mask = 1ULL << SW1_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    }};
    ESP_ERROR_CHECK(gpio_config(&button));
    // Preload OFF before enabling the output to avoid an unintended ON pulse.
    fan_write(false);
    gpio_config_t output = {{
        .pin_bit_mask = 1ULL << FAN_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    }};
    ESP_ERROR_CHECK(gpio_config(&output));
    fan_write(false);
    bool fan_on = false;
    int last_sample = 0, stable_sample = 0, stable_count = 0;
    ESP_LOGI(TAG, "Fan OFF ({port})");
    while (1) {{
        // Use logical pressed=1 in both firmware and simulation.
        int sample = gpio_get_level(SW1_GPIO) == 0;
        if (sample != last_sample) {{
            last_sample = sample;
            stable_count = 0;
        }} else if (stable_count < STABLE_SAMPLES) {{
            ++stable_count;
        }}
        if (stable_count == STABLE_SAMPLES && stable_sample != sample) {{
            stable_sample = sample;
            if (stable_sample) {{
                fan_on = !fan_on;
                fan_write(fan_on);
                ESP_LOGI(TAG, "Fan %s", fan_on ? "ON" : "OFF");
            }}
        }}
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
    }}
}}
"#,
        gpio = port.gpio(),
        on = port.on_level(),
        off = 1 - port.on_level(),
        port = port.key()
    )
}

fn var(name: &str) -> Value {
    json!({"kind":"var","name":name})
}
fn binary(operator: &str, left: Value, right: Value) -> Value {
    json!({"kind":"binary","operator":operator,"left":left,"right":right})
}
fn set(name: &str, value: Value) -> Value {
    json!({"op":"set","name":name,"value":value})
}
pub fn program(port: FanPort) -> Value {
    json!({"version":1,"steps":[
        set("fan_on",json!(false)),set("last_sample",json!(0)),set("stable_sample",json!(0)),set("stable_count",json!(0)),
        {"op":"output","port":port.key(),"value":0},
        {"op":"repeat","body":[
            set("sample",json!({"kind":"sensor","name":"s1"})),
            {"op":"if","condition":binary("!=",var("sample"),var("last_sample")),"then":[set("last_sample",var("sample")),set("stable_count",json!(0))],
             "else":[{"op":"if","condition":binary("<",var("stable_count"),json!(3)),"then":[set("stable_count",binary("+",var("stable_count"),json!(1)))]}]},
            {"op":"if","condition":binary("&&",binary("==",var("stable_count"),json!(3)),binary("!=",var("stable_sample"),var("sample"))),
             "then":[set("stable_sample",var("sample")),{"op":"if","condition":var("stable_sample"),"then":[set("fan_on",json!({"kind":"unary","operator":"!","value":var("fan_on")})),{"op":"output","port":port.key(),"value":var("fan_on")}]}]},
            {"op":"delay","ms":10}
        ]}
    ],"unsupported":[]})
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn each_recipe_uses_its_physical_pin_and_matching_simulator_port() {
        for port in [FanPort::Usb, FanPort::Out1, FanPort::Out2] {
            let c = firmware(port);
            assert!(c.contains(&format!("#define FAN_GPIO GPIO_NUM_{}", port.gpio())));
            assert!(c.contains("ESP_LOGI") && c.contains("STABLE_SAMPLES 3"));
            let js = crate::simulation_compiler::compile(&program(port).to_string()).unwrap();
            assert!(js.contains(&format!("ports.{}(", port.api())));
            for other in [FanPort::Usb, FanPort::Out1, FanPort::Out2] {
                if other != port {
                    let forbidden = format!("ports.{}(", other.api());
                    assert!(!js.contains(&forbidden));
                }
            }
        }
        assert!(firmware(FanPort::Usb).contains("#define FAN_ON_LEVEL 0"));
        assert!(firmware(FanPort::Usb).contains("#define FAN_OFF_LEVEL 1"));
    }
}

#[cfg(test)]
mod fixture_tests {
    #[test]
    fn runtime_fixtures_are_built_from_the_real_recipe_and_compiler() {
        let recipes: serde_json::Value =
            serde_json::from_str(include_str!("../../simulator-tests/fan-recipes.json")).unwrap();
        for port in [
            super::FanPort::Usb,
            super::FanPort::Out1,
            super::FanPort::Out2,
        ] {
            assert_eq!(recipes[port.key()]["firmware"], super::firmware(port));
            assert_eq!(recipes[port.key()]["program"], super::program(port));
            assert_eq!(
                recipes[port.key()]["source"],
                crate::simulation_compiler::compile(&super::program(port).to_string()).unwrap()
            );
        }
    }
}
