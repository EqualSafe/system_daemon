# DO NOT USE AS A MAIN SETUP
# THIS WILL SETUP A VENV
# AND WE WANT THE SYSTEM
# TO HAVE THE LIBRARIES
# GLOBALLY

sudo apt install \
    libdbus-1-dev \
    libgirepository1.0-dev \
    gcc \
    libcairo2-dev \
    pkg-config \
    python3-dev \
    gir1.2-gtk-3.0
python3 -m venv .venv
source .venv/bin/activate
pip install \
    paho-mqtt \
    dbus-python \
    PyGObject

