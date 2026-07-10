import { useEffect, useState } from "react";
import { useEmulatorStore } from "../store/emulatorStore";

let socket: WebSocket | null = null;

export function useWebSocket() {
    const [ws, setWs] = useState<WebSocket | null>(socket);

    const { loadMemory, applyBackendState } = useEmulatorStore();

    useEffect(() => {
        // Already connected (or connecting)
        if (socket) {
            setWs(socket);
            return;
        }

        socket = new WebSocket("ws://localhost:8080");

        socket.onopen = () => {
            console.log("Connected");
            setWs(socket);
        };

        socket.onclose = () => {
            console.log("Disconnected");
            socket = null;
            setWs(null);
        };

        socket.onerror = (e) => {
            console.error("WebSocket error", e);
        };

        socket.onmessage = (event) => {
            const message = JSON.parse(event.data);

            console.log(message);

            switch (message.type) {
                case "memory":
                    console.log("Memory received:", message.data);
                    loadMemory(JSON.parse(message.data));
                    break;

                case "registers":
                    console.log("Registers received:", message.data);
                    break;

                case "state":
                    console.log("State received:", message.data);
                    applyBackendState(JSON.parse(message.data));
                    break;

                default:
                    console.warn("Unknown message type:", message.type);
            }
        };

        setWs(socket);

        // Don't close the shared socket when a component unmounts.
        return () => {};
    }, [loadMemory, applyBackendState]);

    return ws;
}