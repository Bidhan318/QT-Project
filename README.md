# NovaChat

A lightweight LAN-based chat application built with Qt/C++ that enables real-time messaging between multiple clients through a central server.

## Overview

NovaChat is a client-server chat application designed for local area networks. It uses UDP for service discovery and presence announcements, while TCP handles reliable message delivery.

## Architecture

### Server
- Broadcasts its presence via UDP every 2 seconds
- Listens for client connections on TCP
- Routes messages between clients (broadcast & private)
- Manages client tabs and tracks connected users
- Can disconnect individual clients or shut down completely

### Client
- Automatically discovers servers via UDP broadcasts
- Announces presence to other clients every 3 seconds
- Connects to server via TCP for messaging
- Supports broadcast messages (to all) and private messages
- Shows system tray notifications for new messages
- Updates active user list in real-time

## Key Features

- **Auto-Discovery**: Clients automatically find and connect to available servers
- **Dual Messaging**: Send messages to everyone or specific users
- **Real-Time Presence**: Active user list updates as clients join/leave
- **Server Controls**: Kick users or broadcast server-wide messages
- **Desktop Notifications**: Get notified even when window is minimized
- **File Attachments**: Attach files up to 2GB to private messages

## Important Notes

⚠️ **Server Dependency**: Messaging requires an active server. If the server disconnects:
- All clients are notified via TCP
- Messaging functionality is disabled
- Clients must reconnect when server restarts

⚠️ **Network Requirements**:
- All devices must be on the same local network
- Ports used: `PORT` (UDP) and `TCP_PORT` (TCP) - defined in `port.h`
- Firewall must allow traffic on these ports

⚠️ **Logout Handling**: 
- Window close, logout button, and kick all properly notify other clients
- Active lists update immediately via UDP departure announcements

## How It Works

1. **Server starts** → Broadcasts presence via UDP
2. **Client starts** → Discovers server and connects via TCP
3. **Client announces** → Sends presence to other clients via UDP
4. **Messaging** → All messages route through server via TCP
5. **Logout** → Sends TCP logout to server + UDP departure to clients

## Protocol Summary

**UDP Messages:**
- `SERVER_DISCOVERY:IP` - Server announces its presence
- `CLIENT_ANNOUNCE:username` - Client announces it's online
- `CLIENT_DEPARTURE:username` - Client announces it's leaving

**TCP Messages:**
- `LOGIN:username` - Client registers with server
- `LOGOUT:username` - Client disconnects gracefully
- `BROADCAST:sender:message` - Message to all users
- `PRIVATE:recipient:sender:message` - Message to specific user
- `SERVER:message` - Server announcement
- `SERVER_SHUTDOWN` - Server is closing
- `KICKED` - User has been disconnected by server

## Building & Running

1. Build the project using Qt Creator or qmake
2. Configure ports in `port.h` if needed
3. Start the **Server** application first
4. Start one or more **Client** applications
5. Log in with a username and start chatting!

---

**Note**: This is a LAN-only application. It does not support internet-based communication or encryption.
