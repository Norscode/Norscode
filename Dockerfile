FROM debian:bookworm-slim AS runtime

WORKDIR /opt/norscode

RUN apt-get update \
    && apt-get install -y --no-install-recommends bash ca-certificates libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN ./bin/nc run tools/build-bootstrap-binary.no

FROM python:3.12-slim AS runtime

ENV PYTHONUNBUFFERED=1
WORKDIR /opt/norscode

COPY --from=build /opt/norscode /opt/norscode

VOLUME ["/work"]
EXPOSE 8000

ENTRYPOINT ["/opt/norscode/bin/nc"]
CMD ["serve", "/work/examples/web_routes.no", "--host", "0.0.0.0", "--port", "8000", "--production"]
