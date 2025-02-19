void TerminalShooterClient::run() {
    while (running) {
        Packet packet;
        if (connection.receive(packet)) {
            processIncomingPacket(packet);
        }
        // Possibly add a small sleep to prevent 100% CPU usage or integrate with a UI refresh mechanism
    }
} 