// Provider events may carry a JSON object or a serialized JSON tool response.
export function toolResultFailed(result: unknown): boolean {
    if (typeof result === "string") {
        try { return toolResultFailed(JSON.parse(result)); } catch { return false; }
    }
    return !!result && typeof result === "object" && "error" in result;
}


// Readable error text for tool cards; backend API history keeps its original JSON.
export function toolResultErrorMessage(result: unknown): string {
    if (typeof result === "string") {
        try { return toolResultErrorMessage(JSON.parse(result)); } catch { return result; }
    }
    if (!result || typeof result !== "object" || !("error" in result)) return "";
    const error = (result as { error: unknown }).error;
    return typeof error === "string" ? error : JSON.stringify(error) || "Tool failed";
}
