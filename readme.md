# Hello Buttons

This project sets up a minimal firmware app for the Stick C Plus 2 that:

- beeps once when a button is pressed
- plays a double high-pitched beep on a long press
- shows the pressed button name on the display

Two buttons are used in this project:
- top
- front

## PlatformIO setup

1. Open this folder in VS Code.
2. Install the PlatformIO extension if you have not already.
3. Run the project tasks or build with:
   `export PATH="$HOME/Library/Python/3.13/bin:$PATH" && pio run`

## Flashing to the board

Use the PlatformIO upload task or run:
`export PATH="$HOME/Library/Python/3.13/bin:$PATH" && pio run --target upload`

