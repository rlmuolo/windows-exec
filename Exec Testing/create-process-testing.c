#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <processthreadsapi.h>
#include "collectd.h"

#include "common.h"
#include "plugin.h"

#include "utils_cmd_putnotif.h"
#include "utils_cmd_putval.h"


void process_status ( HANDLE hProcess, int pid )
{
	DWORD status;
	LPDWORD exit_code;
	status = WaitForSingleObject( hProcess, INFINITE);
	switch (status)
	{
		case WAIT_ABANDONED:
			printf("Owning process terminated without releasing mutext object.");
			break;
		case WAIT_OBJECT_0:
			printf("The child thread state was signaled");
			break;
		case WAIT_TIMEOUT:
			printf("Wait timeout reached");
			break;
		case WAIT_FAILED:
			printf("Execution has failed %u\n", GetLastError());
			break;
	}
	CloseHandle(hProcess);
}
void _tmain( int argc, TCHAR *argv[] )
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD pid;
	
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	
	if ( argc != 2)
	{
		printf("Usage: %s [cmdline]\n", argv[0]);
		return;
	}
	
	// Start the Child Process
	if ( !CreateProcess( NULL, // No module name (use command line)
		argv[1], // Command Line
		NULL,    // Process handle not inheritable
		NULL,    // Thread handle not inheritable
		FALSE,   // Set handle inheritance to FALSE
		0,       // No Creation Flags
		NULL,	 // Use parent's environment block
		NULL,    // Use parent's starting directory
		&si,     // Pointer to STARTUPINFO structure
		&pi )    // Pointer to PROCESS_INFORMATION structure
	)
		{
			printf( "CreateProcess failed (5d).\n", GetLastError());
			return;
		}
		pid = GetProcessId(pi.hProcess);
		printf("Process id is %d\n", pid);
	process_status(pi.hProcess, pid);
}

static int exec_config_exec(oconfig_item_t *ci) /* {{{ */
{
  program_list_t *pl;
  char buffer[128];
  int i;

  if (ci->children_num != 0) {
    WARNING("exec plugin: The config option `%s' may not be a block.", ci->key);
    return -1;
  }
  if (ci->values_num < 2) {
    WARNING("exec plugin: The config option `%s' needs at least two "
            "arguments.",
            ci->key);
    return -1;
  }
  if ((ci->values[0].type != OCONFIG_TYPE_STRING) ||
      (ci->values[1].type != OCONFIG_TYPE_STRING)) {
    WARNING("exec plugin: The first two arguments to the `%s' option must "
            "be string arguments.",
            ci->key);
    return -1;
  }

  pl = calloc(1, sizeof(*pl));
  if (pl == NULL) {
    ERROR("exec plugin: calloc failed.");
    return -1;
  }

  if (strcasecmp("NotificationExec", ci->key) == 0)
    pl->flags |= PL_NOTIF_ACTION;
  else
    pl->flags |= PL_NORMAL;

  pl->user = strdup(ci->values[0].value.string);
  if (pl->user == NULL) {
    ERROR("exec plugin: strdup failed.");
    sfree(pl);
    return -1;
  }

  pl->group = strchr(pl->user, ':');
  if (pl->group != NULL) {
    *pl->group = '\0';
    pl->group++;
  }

  pl->exec = strdup(ci->values[1].value.string);
  if (pl->exec == NULL) {
    ERROR("exec plugin: strdup failed.");
    sfree(pl->user);
    sfree(pl);
    return -1;
  }

  pl->argv = calloc(ci->values_num, sizeof(*pl->argv));
  if (pl->argv == NULL) {
    ERROR("exec plugin: calloc failed.");
    sfree(pl->exec);
    sfree(pl->user);
    sfree(pl);
    return -1;
  }

  {
    char *tmp = strrchr(ci->values[1].value.string, '/');
    if (tmp == NULL)
      sstrncpy(buffer, ci->values[1].value.string, sizeof(buffer));
    else
      sstrncpy(buffer, tmp + 1, sizeof(buffer));
  }
  pl->argv[0] = strdup(buffer);
  if (pl->argv[0] == NULL) {
    ERROR("exec plugin: strdup failed.");
    sfree(pl->argv);
    sfree(pl->exec);
    sfree(pl->user);
    sfree(pl);
    return -1;
  }

  for (i = 1; i < (ci->values_num - 1); i++) {
    if (ci->values[i + 1].type == OCONFIG_TYPE_STRING) {
      pl->argv[i] = strdup(ci->values[i + 1].value.string);
    } else {
      if (ci->values[i + 1].type == OCONFIG_TYPE_NUMBER) {
        snprintf(buffer, sizeof(buffer), "%lf", ci->values[i + 1].value.number);
      } else {
        if (ci->values[i + 1].value.boolean)
          sstrncpy(buffer, "true", sizeof(buffer));
        else
          sstrncpy(buffer, "false", sizeof(buffer));
      }

      pl->argv[i] = strdup(buffer);
    }

    if (pl->argv[i] == NULL) {
      ERROR("exec plugin: strdup failed.");
      break;
    }
  } /* for (i) */

  if (i < (ci->values_num - 1)) {
    while ((--i) >= 0) {
      sfree(pl->argv[i]);
    }
    sfree(pl->argv);
    sfree(pl->exec);
    sfree(pl->user);
    sfree(pl);
    return -1;
  }

  for (i = 0; pl->argv[i] != NULL; i++) {
    DEBUG("exec plugin: argv[%i] = %s", i, pl->argv[i]);
  }

  pl->next = pl_head;
  pl_head = pl;

  return 0;
} /* int exec_config_exec }}} */

static int exec_config(oconfig_item_t *ci) /* {{{ */
{
  for (int i = 0; i < ci->children_num; i++) {
    oconfig_item_t *child = ci->children + i;
    if ((strcasecmp("Exec", child->key) == 0) ||
        (strcasecmp("NotificationExec", child->key) == 0))
      exec_config_exec(child);
    else {
      WARNING("exec plugin: Unknown config option `%s'.", child->key);
    }
  } /* for (i) */

  return 0;
} /* int exec_config }}} */
