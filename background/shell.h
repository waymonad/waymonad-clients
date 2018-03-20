#ifndef _BACKGROUND_SHELL_H_
#define _BACKGROUND_SHELL_H_

#include <wayland-util.h>

struct background_shell;
struct background_shell *bind_shell(struct registry *registry, const char *path);
void repick_all(struct background_shell *shell);

#endif /* _BACKGROUND_SHELL_H_ */
