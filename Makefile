build:
	docker buildx build \
      --platform linux/amd64 \
      -t acherenovich/snake-server:latest \
      --push \
      .