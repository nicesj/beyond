package com.samsung.android.beyond.inference;

public class PeerInfo {

    private final String host;
    private final int port;
    private String uuid;

    // TODO:
    // List of runtimes

    public PeerInfo(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public PeerInfo(String host, int port, String uuid) {
        this.host = host;
        this.port = port;
    }

    public String getHost() {
        return host;
    }

    public int getPort() {
        return port;
    }

    public String getUuid() {
        return uuid;
    }
}
