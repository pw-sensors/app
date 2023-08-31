# pw_sensor
A workspace for developing a sensor module for Pigweed

## Getting started

### Initial, one-time setup

```bash
$ pip install west
$ west init -l manifest
$ west update
$ . pigweed/bootstrap.sh
$ pip install -r zephyr/scripts/requirements.txt
```

### Activating the development environment

After the initial one-time setup, run these steps
to re-activate your development environment:

```bash
$ . pigweed/activate.sh
```

### Build the project

```bash
$ west build -p -b native_posix app
```

