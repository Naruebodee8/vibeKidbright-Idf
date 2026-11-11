export interface SimulatorEvent {
  protocolVersion: 1;
  sessionId: string;
  requestId: string | null;
  runId: string | null;
  type: string;
  payload: Record<string, unknown>;
}

export class SimulatorAdapter {
  private pending = new Map<string, { resolve: (event: SimulatorEvent) => void; reject: (error: Error) => void; timer: ReturnType<typeof setTimeout> }>();
  private closed = false;
  private listener = (event: MessageEvent) => {
    if (event.origin !== this.origin || event.source !== this.target) return;
    const data = event.data;
    if (!data || data.protocolVersion !== 1 || data.sessionId !== this.sessionId || typeof data.type !== "string" ||
      !data.payload || typeof data.payload !== "object" || Array.isArray(data.payload)) return;
    const message = data as SimulatorEvent;
    if (typeof message.requestId === "string") {
      const request = this.pending.get(message.requestId);
      if (request) {
        clearTimeout(request.timer);
        this.pending.delete(message.requestId);
        if (message.type === "ERROR") request.reject(new Error(String(message.payload.message || "Simulator error")));
        else request.resolve(message);
      }
    }
    this.onEvent(message);
  };

  constructor(private target: Window, private origin: string, private sessionId: string, private onEvent: (event: SimulatorEvent) => void) {
    window.addEventListener("message", this.listener);
  }

  request(type: string, payload: Record<string, unknown> = {}, runId: string | null = null, timeout = 10000): Promise<SimulatorEvent> {
    if (this.closed) return Promise.reject(new Error("Simulator connection is closed"));
    const requestId = crypto.randomUUID();
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => { this.pending.delete(requestId); reject(new Error("Simulator did not respond: " + type)); }, timeout);
      this.pending.set(requestId, { resolve, reject, timer });
      try {
        this.target.postMessage({ protocolVersion: 1, sessionId: this.sessionId, requestId, runId, type, payload }, this.origin);
      } catch (error) {
        clearTimeout(timer);
        this.pending.delete(requestId);
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  close() {
    if (this.closed) return;
    this.closed = true;
    window.removeEventListener("message", this.listener);
    for (const request of this.pending.values()) { clearTimeout(request.timer); request.reject(new Error("Simulator closed")); }
    this.pending.clear();
  }
}
