# Test phase – giving feedback

[Deutsch](TESTPHASE.md) · **English**

Thanks for testing the app! This is how you help the most.

## Reporting a bug

1. On GitHub, go to **Issues → New issue** in the repository and choose **Bug report / Fehlerbericht**. The form asks for everything needed.
2. No GitHub account? Then just send the same details to Basti in a message.

What always matters: **what you did, what happened, and what you expected.** A photo of the display or a screenshot of the settings page helps a lot.

## Attaching the log

The app writes everything important to the file **`usb-display.log`**:

- Version 2.0 regular installation: `%LOCALAPPDATA%\FuchsLabs\USB-Display\usb-display.log`.
- A custom `--log FILE` overrides the path.

The file contains **no tokens or keys**. It may contain names of your friends, servers and games, though. Have a quick look and replace anything you don't want to share before attaching it. For more detail, start the app once with `-v` (shortcut or command prompt: `FuchsLabs-USB-Display.exe -v`).

**Never send:** the file `config.toml`, your bot token, Steam or Xbox keys.

## Wishes and ideas

Under **Issues → New issue → Idea or feedback / Idee oder Rückmeldung** you can tell us anything you like, miss or find awkward.

## Windows reboot test

After signing in, check the physical display, fox tray icon and whether settings open in their own Windows window. Report the app version, whether images keep updating and any error. The first test failed; confirmation of the corrected installation is pending.
