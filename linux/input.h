#ifndef LINUX_INPUT_H
#define LINUX_INPUT_H

void input_host_read(unsigned short *button, signed char *stickX, signed char *stickY);

// Scripted input (DKR_INPUT_SCRIPT): the scripted state of controller `port`
// (0-based), the number of ports the script drives (0 without a script), and
// whether the script's AUTO token is held, which has the game drive human
// racers with the AI's inputs.
void input_host_read_port(int port, unsigned short *button, signed char *stickX, signed char *stickY);
int input_script_ports(void);
int input_script_autopilot(void);

#endif
