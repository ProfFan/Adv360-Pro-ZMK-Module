FROM docker.io/zmkfirmware/zmk-build-arm:stable

WORKDIR /app

COPY config/west.yml config/west.yml

# West Init
RUN west init -l config
# West Update
RUN west update
# West Zephyr export
RUN west zephyr-export

COPY CMakeLists.txt Kconfig ./
COPY zephyr zephyr
COPY boards boards
COPY dts dts
COPY include include
COPY src src
COPY bin/build.sh ./

CMD ["./build.sh"]
