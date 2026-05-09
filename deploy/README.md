# Snake Docker Stack

`stack.yml` is intended for Docker Swarm and does not build images on the server.
Images are published to GitHub Container Registry first, then deployed with
`docker stack deploy`.

Required GitHub Actions secrets in `snake-server`:

- `DEPLOY_HOST` - cloud server host or IP.
- `DEPLOY_USER` - SSH user on the cloud server.
- `DEPLOY_SSH_KEY` - private SSH key for deployment.
- `DEPLOY_SSH_PORT` - optional SSH port, defaults to `22`.
- `GHCR_USERNAME` - GitHub username used by the server for `docker login`.
- `GHCR_TOKEN` - token with permission to read GHCR packages.
- `MYSQL_HOST` - existing MySQL host.
- `MYSQL_PORT` - existing MySQL port, defaults to `3306`.
- `MYSQL_USER` - MySQL user.
- `MYSQL_PASSWORD` - MySQL password.
- `MYSQL_DATABASE` - MySQL database.
- `MYSQL_THREADS` - optional MySQL pool size, defaults to `1`.
- `SNAKE_PUBLIC_HOST` - public host/IP that game clients should use for UDP.

One-time server setup:

```sh
docker swarm init
```

Open these ports on the cloud firewall:

- `9100/tcp`
- `7778-7782/udp`
- `7788-7792/udp`
- `7798-7802/udp`
