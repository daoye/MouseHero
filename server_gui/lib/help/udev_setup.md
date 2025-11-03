# Linux uinput Permission Setup Guide

## Why Do We Need to Set Up Permissions?

MouseHero uses the Linux kernel's uinput interface to control mouse and keyboard input. By default, the `/dev/uinput` device is only accessible by the root user. For security reasons, we don't want to run the application with root privileges, so we need to grant access to regular users through udev rules.



### Step 1: Add Your User to the input Group

```bash
sudo usermod -aG input $USER
```


### Step 2: Create the udev Rule File

Open a terminal and create the rule file using a text editor:

```bash
sudo nano /etc/udev/rules.d/99-mousehero-uinput.rules
```


### Step 3: Add Rule Content

Add the following content to the file:

```
KERNEL=="uinput", SUBSYSTEM=="misc", MODE="0660", GROUP="input"
```

Save and exit the editor (Nano: Ctrl+O, Enter, Ctrl+X).


### Step 4: Reload udev Rules

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```


### Step 5: Log Out and Back In

Log out of your current session and log back in, or reboot your computer.

**Important**: You must log out and back in for the group membership to take effect.


### Step 6: Verify Permissions

Return to the MouseHero application and verify that input functionality works correctly.




## Frequently Asked Questions

### Q: The /dev/uinput device does not exist

**A:** You need to load the uinput kernel module:

```bash
sudo modprobe uinput
```

To make the module load automatically at boot, add it to `/etc/modules-load.d/uinput.conf`:

```bash
echo "uinput" | sudo tee /etc/modules-load.d/mousehero-uinput.conf
```

### Q: Running in a container or sandbox environment

**A:** Containers (such as Docker, Flatpak, Snap) may block access to the `/dev/uinput` device. You need to:
- Run the container with device passthrough
- Or run MouseHero on the host machine

### Q: How to revoke permission settings?


**A:** Delete the udev rule file and reload:

```bash
sudo rm /etc/udev/rules.d/99-mousehero-uinput.rules
sudo rm /etc/modules-load.d/mousehero-uinput.conf
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo gpasswd -d $USER input
```


## Security Notes

- ✅ The application itself **does not need** root privileges to run
- ✅ Only a **one-time** sudo operation is needed to set up udev rules
- ✅ Permissions are granted only to the currently logged-in user or specific user groups
- ⚠️ Users with uinput access can simulate keyboard and mouse input
- ⚠️ Please ensure this feature is only used on trusted systems
