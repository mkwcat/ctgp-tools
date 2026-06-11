#include "efstool.h"

#include "util.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* efstool_executable_name = "efstool";

struct efstool_string_enum {
    const char* string;
    int         value;
};

typedef const struct efstool_string_enum efstool_enum_list[];

enum efstool_opt_type {
    efstool_opt_type_none,
    efstool_opt_type_bool,
    efstool_opt_type_number,
    efstool_opt_type_string,
    efstool_opt_type_path,
    efstool_opt_type_enum,
    efstool_opt_type_var_arg,
    efstool_opt_type_count,
};

struct efstool_opt_value {
    long                      number;
    const char*               string;
    struct efstool_opt_value* next;
};

enum efstool_opt {
    efstool_opt_help,
    efstool_opt_verbose,
    efstool_opt_silent,
    efstool_opt_crypt_type,
    efstool_opt_count,
};

struct efstool_option {
    unsigned char                     opt;
    unsigned char                     short_name;
    unsigned char                     type;
    unsigned char                     category;
    unsigned char                     max_count;
    const char*                       long_name;
    const char*                       description;
    const struct efstool_string_enum* enums;
};

enum efstool_cmd {
    efstool_cmd_none,
    efstool_cmd_encrypt,
    efstool_cmd_decrypt,
    efstool_cmd_pack,
    efstool_cmd_unpack,
    efstool_cmd_update,
    efstool_cmd_count,
};

struct efstool_command {
    unsigned char cmd;
    unsigned char category;
    const char*   name;
    const char*   usage;
    const char*   description;
    const char*   supported_options;
    const char*   required_options;
    const char*   positional_args;
};

enum efstool_opt_category {
    efstool_opt_category_general,
    efstool_opt_category_blob,
    efstool_opt_category_update,
    efstool_opt_category_count,
};

enum efstool_crypt_type {
    efstool_crypt_type_blob,
    efstool_crypt_type_jelly,
};

static const struct efstool_command efstool_commands[] = {
    // clang-format off
    [efstool_cmd_encrypt] = {
        .cmd               = efstool_cmd_encrypt,
        .name              = "encrypt",
        .category          = efstool_opt_category_blob,
        .usage             = "<input file> <output file>",
        .description       = "Encrypt a raw EFS file into blob.bin or jelly.bin",
        .positional_args   = (const char[]) {efstool_opt_type_path, efstool_opt_type_path, 0},
        .supported_options = (const char[]) {efstool_opt_crypt_type, 0},
    },
    [efstool_cmd_decrypt] = {
        .cmd               = efstool_cmd_decrypt,
        .name              = "decrypt",
        .category          = efstool_opt_category_blob,
        .usage             = "<input file> <output file>",
        .description       = "Decrypt blob.bin or jelly.bin into its raw EFS data",
        .positional_args   = (const char[]) {efstool_opt_type_path, efstool_opt_type_path, 0},
        .supported_options = (const char[]) {efstool_opt_crypt_type, 0},
    },
    [efstool_cmd_pack] = {
        .cmd               = efstool_cmd_pack,
        .name              = "pack",
        .category          = efstool_opt_category_blob,
        .usage             = "<input directory> <output file>",
        .description       = "Pack a directory into blob.bin or jelly.bin",
        .positional_args   = (const char[]) {efstool_opt_type_path, efstool_opt_type_path, 0},
        .supported_options = (const char[]) {efstool_opt_crypt_type, 0},
    },
    [efstool_cmd_unpack] = {
        .cmd               = efstool_cmd_unpack,
        .name              = "unpack",
        .category          = efstool_opt_category_blob,
        .usage             = "<input file> <output directory>",
        .description       = "Unpack blob.bin or jelly.bin into a directory",
        .positional_args   = (const char[]) {efstool_opt_type_path, efstool_opt_type_path, 0},
        .supported_options = (const char[]) {efstool_opt_crypt_type, 0},
    },
    [efstool_cmd_update] = {
        .cmd               = efstool_cmd_update,
        .name              = "update",
        .category          = efstool_opt_category_update,
        .usage             = "[update.dif ...]",
        .positional_args   = (const char[]) {efstool_opt_type_path, efstool_opt_type_var_arg},
        .supported_options = (const char[]) {0},
    },
    // clang-format on
};

static const struct efstool_option efstool_options[] = {
    // clang-format off
    [efstool_opt_help] = {
        .opt         = efstool_opt_help,
        .category    = efstool_opt_category_general,
        .short_name  = 'h',
        .long_name   = "help",
        .description = "Show this help message and exit",
        .type        = efstool_opt_type_bool,
    },
    [efstool_opt_verbose] = {
        .opt         = efstool_opt_verbose,
        .category    = efstool_opt_category_general,
        .short_name  = 'v',
        .long_name   = "verbose",
        .description = "Print verbose output",
        .type        = efstool_opt_type_bool,
    },
    [efstool_opt_silent] = {
        .opt         = efstool_opt_silent,
        .category    = efstool_opt_category_general,
        .short_name  = 's',
        .long_name   = "silent",
        .description = "Don't print anything except errors",
        .type        = efstool_opt_type_bool,
    },
    [efstool_opt_crypt_type] = {
        .opt         = efstool_opt_crypt_type,
        .category    = efstool_opt_category_blob,
        .short_name  = 't',
        .long_name   = "type",
        .description = "<blob, jelly> Specify the encryption type",
        .type        = efstool_opt_type_enum,
        .max_count   = 1,
        .enums       = (efstool_enum_list) {
            {"blob",  efstool_crypt_type_blob},
            {"jelly", efstool_crypt_type_jelly},
            {NULL, 0},
        },
    },
    // clang-format on
};

static const char* efstool_usage_category_names[] = {
    [efstool_opt_category_general] = "General options",
    [efstool_opt_category_blob]    = "Extraction options",
    [efstool_opt_category_update]  = "Updater options",
};

static const struct efstool_command* efstool_command = NULL;

static struct efstool_opt_value* efstool_option_values[efstool_opt_count]      = {0};
static struct efstool_opt_value* efstool_option_values_tail[efstool_opt_count] = {0};
static int                       efstool_option_counts[efstool_opt_count]      = {0};

static struct efstool_opt_value* efstool_positional_args      = NULL;
static struct efstool_opt_value* efstool_positional_args_tail = NULL;
static int                       efstool_positional_arg_count = 0;
static int                       efstool_positional_arg_index = 0;

static bool exit_unknown_option(
    const char* arg
) {
    printf("Unknown option: %s\n", arg);
    printf("Use --help to see available options.\n");
    return false;
}

static const struct efstool_option* find_option_by_long_name(
    const char* long_name
) {
    size_t      length;
    const char* end = strchr(long_name, '=');
    if (end != NULL) {
        length = end - long_name;
    } else {
        length = strlen(long_name);
    }

    for (size_t i = 0; i < util_array_size(efstool_options); i++) {
        const char* const candidate = efstool_options[i].long_name;
        if (candidate && strncmp(long_name, candidate, length) == 0 && candidate[length] == '\0') {
            return &efstool_options[i];
        }
    }
    return NULL;
}

static const struct efstool_option* find_option_by_short_name(
    char short_name
) {
    for (size_t i = 0; i < util_array_size(efstool_options); i++) {
        if (efstool_options[i].short_name == short_name) {
            return &efstool_options[i];
        }
    }
    return NULL;
}

static bool parse_option_value(
    const struct efstool_option* opt, const char* value
) {
    long                  number = 0;
    enum efstool_opt_type type;
    if (opt != NULL) {
        type = (enum efstool_opt_type) opt->type;
        efstool_option_counts[opt->opt]++;
    } else {
        assert(efstool_command != NULL);
        const char* const pos_args = efstool_command->positional_args;
        if (pos_args == NULL || pos_args[efstool_positional_arg_index] == 0) {
            printf(
                "Exceeded maximum count of positional arguments for '%s': %d\n",
                efstool_command->name, efstool_positional_arg_index
            );
            return false;
        }
        type = (enum efstool_opt_type) pos_args[efstool_positional_arg_index];
        if (type == efstool_opt_type_var_arg) {
            type = (enum efstool_opt_type) pos_args[efstool_positional_arg_index - 1];
        } else {
            efstool_positional_arg_index++;
        }
        efstool_positional_arg_count++;
    }

    switch (type) {
    case efstool_opt_type_none:
    case efstool_opt_type_var_arg:
    case efstool_opt_type_count:
    default:
        assert(false && "Invalid option type in parse_option_value");

    case efstool_opt_type_bool:
        // Only internally passed
        number = 1;
        break;

    case efstool_opt_type_number: {
        char* end;
        errno   = 0;
        number  = strtol(value, &end, 10);
        int err = errno;
        if (end == value || *end != '\0' || (err != 0 && err != ERANGE)) {
            printf("Invalid value for integer option: %s\n", value);
            return false;
        }
        if (err == ERANGE || number < -2147483648l || number > 2147483647l) {
            printf("Value out of range for integer option: %s\n", value);
            return false;
        }
        break;
    }

    case efstool_opt_type_string:
    case efstool_opt_type_path:
        break;

    case efstool_opt_type_enum: {
        assert(opt != NULL);
        bool found = false;
        for (const struct efstool_string_enum* list = opt->enums; list->string; list++) {
            if (strcmp(list->string, value) == 0) {
                number = list->value;
                found  = true;
                break;
            }
        }
        if (!found) {
            printf(
                "Unknown value for --%s: %s\n"
                "Accepted values are: ",
                opt->long_name, value
            );
            bool need_comma = false;
            for (const struct efstool_string_enum* list = opt->enums; list->string; list++) {
                printf("%s%s", need_comma ? ", " : "", list->string);
                need_comma = true;
            }
            putchar('\n');
            return false;
        }
        break;
    }
    }

    if (efstool_command == NULL && opt->category != efstool_opt_category_general) {
        printf("Option --%s must not be specified before a command\n", opt->long_name);
        return false;
    }
    if (efstool_command && opt->category != efstool_opt_category_general &&
        !strchr(efstool_command->supported_options, opt->opt)) {
        printf("Option --%s not supported by command: %s\n", opt->long_name, efstool_command->name);
        return false;
    }

    if (opt && opt->max_count && efstool_option_counts[opt->opt] > opt->max_count) {
        printf("Exceeded maximum value count of %d option --%s\n", opt->max_count, opt->long_name);
        return false;
    }

    struct efstool_opt_value* const value_ptr =
        (struct efstool_opt_value*) malloc(sizeof(struct efstool_opt_value));
    value_ptr->string = value;
    value_ptr->number = number;

    if (opt != NULL) {
        struct efstool_opt_value* const tail = efstool_option_values_tail[opt->opt];
        if (tail) {
            tail->next = value_ptr;
        } else {
            efstool_option_values[opt->opt] = value_ptr;
        }
        efstool_option_values_tail[opt->opt] = value_ptr;
    } else {
        struct efstool_opt_value* const tail = efstool_positional_args_tail;
        if (tail) {
            tail->next = value_ptr;
        } else {
            efstool_positional_args = value_ptr;
        }
        efstool_positional_args_tail = value_ptr;
    }

    return true;
}

static int parse_long_option(
    char* const* arg
) {
    const struct efstool_option* opt = find_option_by_long_name(arg[0] + 2);
    if (opt == NULL) {
        exit_unknown_option(arg[0]);
        return 0;
    }
    if (opt->type == efstool_opt_type_bool) {
        // Boolean flags don't take a value. Check if we have a value anyway
        if (strchr(arg[0], '=')) {
            printf("Option --%s does not take a value\n", opt->long_name);
            return 0;
        }
        return parse_option_value(opt, NULL) ? 1 : 0;
    }
    const size_t long_name_length = strlen(opt->long_name);
    if (arg[0][2 + long_name_length] == '=') {
        // --opt=value
        return parse_option_value(opt, arg[0] + 2 + long_name_length + 1) ? 1 : 0;
    }
    // --opt value
    if (arg[1] == NULL) {
        printf("Missing value for option: %s\n", arg[0]);
        return 0;
    }
    return parse_option_value(opt, arg[1]) ? 2 : 0;
}

static int parse_short_option(
    char* const* const arg
) {
    const struct efstool_option* const opt = find_option_by_short_name(arg[0][1]);
    if (opt == NULL) {
        exit_unknown_option(arg[0]);
        return 0;
    }
    if (opt->type == efstool_opt_type_bool) {
        // Boolean flags don't take a value. Check if we have a value anyway
        if (arg[0][2] != '\0') {
            printf("Option -%c does not take a value\n", opt->short_name);
        }
        return parse_option_value(opt, NULL) ? 1 : 0;
    }
    if (arg[0][2]) {
        // -ovalue
        return parse_option_value(opt, arg[0] + 2) ? 1 : 0;
    }
    // -o value
    if (arg[1] == NULL) {
        printf("Missing value for option: %s\n", arg[0]);
        return 0;
    }
    return parse_option_value(opt, arg[1]) ? 2 : 0;
}

static bool parse_arguments(
    const int argc, char* const* const argv
) {
    for (int arg_num = 1; arg_num < argc; arg_num++) {
        const char* arg = argv[arg_num];
        if (arg[0] == '-' && arg[1] == '-') {
            // Long option name
            const int result = parse_long_option(argv + arg_num);
            if (result == 0) {
                return false;
            }
            arg_num += result - 1;
        } else if (arg[0] == '-' && arg[1]) {
            // Short option name
            const int result = parse_short_option(argv + arg_num);
            if (result == 0) {
                return false;
            }
            arg_num += result - 1;
        } else if (efstool_command == NULL) {
            // Command argument
            size_t i;
            for (i = 0; i < util_array_size(efstool_commands); i++) {
                if (efstool_commands[i].name && strcmp(efstool_commands[i].name, arg) == 0) {
                    break;
                }
            }
            if (i == util_array_size(efstool_commands)) {
                printf(
                    "Unknown sub-command: %s\n"
                    "See --help for usage\n",
                    arg
                );
                return false;
            }
            efstool_command = &efstool_commands[i];
        } else {
            // Positional argument
            if (!parse_option_value(NULL, arg)) {
                return false;
            }
        }
    }
    return true;
}

int efstool_print_help(
    void
) {
    printf("Usage: %s <", efstool_executable_name);
    bool need_pipe = false;
    for (size_t cmd = 0; cmd < efstool_cmd_count; cmd++) {
        const char* name = efstool_commands[cmd].name;
        if (name) {
            printf("%s%s", need_pipe ? "|" : "", name);
            need_pipe = true;
        }
    }
    puts("> [options...]\n"); // Two newlines

    for (size_t cat = 0; cat < efstool_opt_category_count; cat++) {
        const char* name = efstool_usage_category_names[cat];
        if (!name) {
            continue;
        }

        printf("%s:\n", name);
        for (size_t cmd = 0; cmd < efstool_cmd_count; cmd++) {
            const struct efstool_command* command = &efstool_commands[cmd];
            if (!command->name || !command->usage || command->category != cat) {
                continue;
            }

            printf("%s %s %s\n", efstool_executable_name, command->name, command->usage);
        }
        putchar('\n');

        bool wrote_opt = false;
        for (size_t opt = 0; opt < efstool_opt_count; opt++) {
            const struct efstool_option* option = &efstool_options[opt];
            if (!option->short_name && !option->long_name) {
                continue;
            }
            if (option->category != cat) {
                continue;
            }

            const char* description = option->description ? option->description : "";
            if (option->short_name) {
                printf("  -%c, --%-9s %s\n", option->short_name, option->long_name, description);
            } else {
                printf("      --%-9s %s\n", option->long_name, description);
            }
            wrote_opt = true;
        }
        if (wrote_opt) {
            putchar('\n');
        }
    }
    return EXIT_FAILURE;
}

static bool efstool_main(
    const int argc, char* const* argv
) {
    if (!parse_arguments(argc, argv)) {
        return false;
    }

    if (efstool_option_values[efstool_opt_help]) {
        efstool_print_help();
        return false;
    }

    return true;
}

int main(
    const int argc, char** const argv
) {
    if (getenv("EFSTOOL_NO_SCREEN_WORKS") == NULL) {
        puts("The screen works");
    }

    return efstool_main(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;
}
