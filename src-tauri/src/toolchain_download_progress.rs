//! One coherent progress/ETA for concurrent archive downloads.
use std::time::Duration;
#[derive(Default)]
struct Part {
    size: Option<u64>,
    downloaded: u64,
}
pub(crate) struct Downloads {
    parts: Vec<Part>,
    last_time: Duration,
    last_bytes: u64,
    peak: u8,
}
pub(crate) struct Snapshot {
    pub percent: u8,
    pub downloaded: u64,
    pub total: Option<u64>,
    pub speed: f64,
    pub eta: Option<f64>,
}
impl Downloads {
    pub fn new(count: usize) -> Self {
        Self {
            parts: (0..count).map(|_| Part::default()).collect(),
            last_time: Duration::ZERO,
            last_bytes: 0,
            peak: 1,
        }
    }
    pub fn set_size(&mut self, part: usize, size: Option<u64>) {
        self.parts[part].size = size;
    }
    pub fn update(
        &mut self,
        part: usize,
        downloaded: u64,
        now: Duration,
        force: bool,
    ) -> Option<Snapshot> {
        self.parts[part].downloaded = self.parts[part].downloaded.max(downloaded);
        let interval = now.saturating_sub(self.last_time);
        if !force && interval < Duration::from_secs(3) {
            return None;
        }
        let done = self.parts.iter().map(|p| p.downloaded).sum::<u64>();
        let total = self
            .parts
            .iter()
            .try_fold(0u64, |sum, p| sum.checked_add(p.size?))
            .filter(|size| *size > 0);
        let speed = if interval.as_secs_f64() > 0.0 {
            done.saturating_sub(self.last_bytes) as f64 / interval.as_secs_f64()
        } else {
            0.0
        };
        let percent = total
            .map(|size| ((done.min(size) as f64 / size as f64) * 49.0) as u8)
            .unwrap_or(self.peak);
        self.peak = self.peak.max(percent).min(49);
        let eta =
            total.and_then(|size| (speed > 0.0).then(|| size.saturating_sub(done) as f64 / speed));
        self.last_bytes = done;
        self.last_time = now.max(self.last_time);
        Some(Snapshot {
            percent: self.peak,
            downloaded: done,
            total,
            speed,
            eta,
        })
    }
}
impl Snapshot {
    pub fn message(&self) -> String {
        let total = self
            .total
            .map(|size| format!("{:.1}", size as f64 / 1_048_576.0))
            .unwrap_or_else(|| "ไม่ทราบขนาดรวม".into());
        let eta = self
            .eta
            .map(|seconds| {
                let seconds = seconds.ceil() as u64;
                format!("{} นาที {} วินาที", seconds / 60, seconds % 60)
            })
            .unwrap_or_else(|| "กำลังคำนวณ".into());
        format!(
            "รวม {:.1}/{} MB · {:.1} MB/s · เหลือประมาณ {}",
            self.downloaded as f64 / 1_048_576.0,
            total,
            self.speed / 1_048_576.0,
            eta
        )
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn combines_both_files_and_calculates_one_eta_from_total_speed() {
        let mut state = Downloads::new(2);
        state.set_size(0, Some(300));
        state.set_size(1, Some(1900));
        assert!(state
            .update(0, 100, Duration::from_secs(1), false)
            .is_none());
        let snapshot = state.update(1, 200, Duration::from_secs(3), false).unwrap();
        assert_eq!(snapshot.downloaded, 300);
        assert_eq!(snapshot.total, Some(2200));
        assert_eq!(snapshot.speed, 100.0);
        assert_eq!(snapshot.eta, Some(19.0));
        assert_eq!(snapshot.percent, 6);
        assert!(state
            .update(0, 150, Duration::from_secs(3), false)
            .is_none());
    }
    #[test]
    fn unknown_sizes_do_not_make_the_bar_wrap_and_completion_never_precedes_extraction() {
        let mut state = Downloads::new(2);
        state.set_size(0, Some(300));
        let unknown = state.update(0, 200, Duration::from_secs(3), false).unwrap();
        assert_eq!(unknown.total, None);
        assert_eq!(unknown.eta, None);
        state.set_size(1, Some(1900));
        let known = state
            .update(1, 1000, Duration::from_secs(6), false)
            .unwrap();
        let duplicate = state.update(1, 100, Duration::from_secs(6), true).unwrap();
        assert_eq!(duplicate.downloaded, known.downloaded);
        assert!(duplicate.percent >= known.percent);
        state.update(0, 300, Duration::from_secs(8), false);
        let complete = state
            .update(1, 1900, Duration::from_secs(9), false)
            .unwrap();
        assert_eq!(complete.percent, 49);
        assert_eq!(complete.eta, Some(0.0));
    }
}
