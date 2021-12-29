// 
// -----------------------------------------------------------------------------
// argparse.h
// -----------------------------------------------------------------------------
//
// Argument parser for main program
//

#include <argp.h>

const char *argp_program_version = "Preview\n";
const char *argp_program_bug_address = "<tylerwayne3@gmail.com>";

struct arguments {
  char *path;
  char delim;
  int headers;
};

static struct argp_option options[] = {
  {"delimiter", 'd', "DELIM", 0, "Use DELIM instead of PIPE"},
  {"no-header", 'h', 0, 0, "Skip header row"},
  {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {

  struct arguments *arguments = state->input;

  switch (key) {
    case 'd':
      arguments->delim = arg[0];
      break;

    case 'h':
      arguments->headers = 0;
      break;

    // Position args
    case ARGP_KEY_ARG:
      // Too many arguments
      if (state->arg_num > 1) argp_usage(state);
      // arguments->args[state->arg_num] = arg;
      arguments->path = arg;
      break;

    case ARGP_KEY_END: 
      // Not enough arguments
      if (state->arg_num < 1) argp_usage(state); 
      break;

    default: 
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static char args_doc[] = "path";
static char doc[] = "preview -- display delimited data for quick investigation";
static struct argp argp = { options, parse_opt, args_doc, doc };
