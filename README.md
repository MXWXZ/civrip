# civrip
GUI to create virtual network for civilization 6 and old school games in multiplayer mode.

简单GUI为文明6及老游戏联机提供虚拟组网。

[中文Readme](README_zh_CN.md)

## Motivation
Civilization 6 and other old school games only support LAN for multiplayer (if you cannot connect to Civilization 6 official server). The most common way is to create a virtual network to simulate a LAN.

Possible solution:
- [Zerotier](https://www.zerotier.com/): Work for most cases. However, when user is behind firewall that does not allow P2P hole punching, the experience is very annoying. Self-hosting moon node is another solution, but UDP QoS is a new challenge for some ISPs. 
- [n2n](https://github.com/ntop/n2n): Similar to Zerotier, but it needs a coordinator server to connect different users and forwarding traffic (if P2P is disabled). It allows the user to force TCP connection instead of UDP.
- VPN: Also work for most cases, but it is too heavy and not allowed in some network environment.

Civilization 6 has another problem that it only scan the default NIC for local rooms, but most solutions based on virtual NIC. We need [WinIPBroadcast](https://github.com/dechamps/WinIPBroadcast) to send IP broadcast packets to all interfaces.

I developed this tool to automate the process for Civilization 6 multiplayer mode (and old school games), based on n2n to overcome network issues.

## Server Deploy
In order to provide better experience, you need to have a VPS or public IP address. Please deploy n2n 3.0 on the server, or use our docker image.

```
vim docker-compose.yml
# modify the port

vim community.list
# add community

docker compose up -d
```

Community acts like the channel on the server, and provide basic authentication. You MUST provide a community list to use civrip.

Example `community.list`
```
civsix
free
```
The server will have two community, `civsix` and `free`.

## Client
Download the latest release on the [release page](https://github.com/MXWXZ/civrip/releases).

1. Execute `civrip.exe`
2. Click `Install WinIPB` or `Install TAP` if status is `UNINST`.
3. Click `Restart WinIPB` or `Restart TAP` if status is `STOPPED` or `UNKNOWN`.
4. Fill in `Server IP` with port, e.g., `1.2.3.4:7654`.
5. Fill in `Community` configured by server, e.g., `civsix`.
6. Fill in `Encrypt Key`, this can be arbitrary, e.g., `114514`.
7. Do not check any network setting and click `Connect`.
8. If server status is `CONNECT`, go and play your game! Otherwise, please check the output by yourself.

### Troubleshooting
These only work if your server status is `CONNECT`, otherwise please solve the server connection problem first.

If you cannot find the room or cannot connect to the room, please follow the steps below. Please check the game to see if you solve the problem after complete each step:

1. Disconnect, uncheck all network settings and connect again.
2. Disconnect, click `Restart WinIPB` and `Restart TAP`, then connect again.
3. Disconnect, check `Disable P2P` and connect again (This will disable P2P to bypass strict NAT network, you need to pay for the bandwidth for your VPS!).
4. Disconnect, check `Disable P2P` and `Force TCP` and connect again (This may solve some UDP QoS problem).
5. Disconnect, check `Disable P2P`, `Force TCP` and `Disable Encrypt` and connect again.
6. Change another network and start from step 1.
7. Restart the computer and start from step 1.
8. Reinstall your Windows system.
9. Buy another computer.

If you can connect, but the connection is not stable, please check `Disable P2P` (and `Force TCP` if possible).

## License
GNU General Public License v3.0