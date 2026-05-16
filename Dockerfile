FROM python:3.12-slim AS build

ENV PYTHONUNBUFFERED=1
WORKDIR /opt/norscode

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential clang sqlite3 libsqlite3-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN ./tools/build-bootstrap-binary.sh

FROM python:3.12-slim AS runtime

ENV PYTHONUNBUFFERED=1
WORKDIR /opt/norscode

COPY --from=build /opt/norscode /opt/norscode

VOLUME ["/work"]
EXPOSE 8000

ENTRYPOINT ["/opt/norscode/dist/norscode"]
CMD ["serve", "/work/examples/web_routes.no", "--host", "0.0.0.0", "--port", "8000", "--production"]
