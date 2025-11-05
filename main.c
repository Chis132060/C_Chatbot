#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#define MAX_KB 200
#define MAX_KEY_LEN 128
#define MAX_RESP 20
#define MAX_RESP_LEN 256
#define LINE_BUF 1024

typedef struct {
    char keyword[MAX_KEY_LEN];
    char responses[MAX_RESP][MAX_RESP_LEN];
    int resp_count;
} KBEntry;

static KBEntry kb[MAX_KB];
static int kb_count = 0;

// Trim whitespace in-place
static void trim(char *s) {
    char *p = s;
    // leading
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    // trailing
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

// Lowercase in-place
static void to_lower(char *s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

// Load knowledge base file. Format per line: keyword:resp1;resp2;resp3
void load_kb(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Warning: could not open knowledge file '%s'\n", filename);
        return;
    }

    char line[LINE_BUF];
    while (fgets(line, sizeof(line), f) && kb_count < MAX_KB) {
        // skip comments and blank lines
        trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        char *sep = strchr(line, ':');
        if (!sep) continue; // malformed

        *sep = '\0';
        char *keyword = line;
        char *resp_list = sep + 1;
        trim(keyword);
        trim(resp_list);
        if (keyword[0] == '\0' || resp_list[0] == '\0') continue;

        strncpy(kb[kb_count].keyword, keyword, MAX_KEY_LEN-1);
        kb[kb_count].keyword[MAX_KEY_LEN-1] = '\0';
        to_lower(kb[kb_count].keyword);

        // split responses by ';'
        int r = 0;
        char *token = strtok(resp_list, ";");
        while (token && r < MAX_RESP) {
            trim(token);
            strncpy(kb[kb_count].responses[r], token, MAX_RESP_LEN-1);
            kb[kb_count].responses[r][MAX_RESP_LEN-1] = '\0';
            r++;
            token = strtok(NULL, ";");
        }
        kb[kb_count].resp_count = r > 0 ? r : 0;
        kb_count++;
    }

    fclose(f);
}

// Find a response for input, or NULL
const char *find_response(const char *input_orig) {
    static char input_copy[LINE_BUF];
    strncpy(input_copy, input_orig, sizeof(input_copy)-1);
    input_copy[sizeof(input_copy)-1] = '\0';
    to_lower(input_copy);

    // exact exit checks
    if (strstr(input_copy, "quit") || strstr(input_copy, "exit") || strstr(input_copy, "bye")) {
        return "Goodbye!";
    }

    for (int i = 0; i < kb_count; ++i) {
        if (strstr(input_copy, kb[i].keyword) != NULL) {
            if (kb[i].resp_count > 0) {
                int idx = rand() % kb[i].resp_count;
                return kb[i].responses[idx];
            }
        }
    }

    return "I'm not sure I understand. Can you rephrase that?";
}

int main(int argc, char **argv) {
    const char *kbfile = "knowledge.txt";
    if (argc > 1) kbfile = argv[1];

    srand((unsigned int)time(NULL));
    load_kb(kbfile);

    printf("C Chatbot: Hello! Type 'quit' or 'exit' to end the conversation.\n");

    char buf[LINE_BUF];
    while (1) {
        printf("You: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            // EOF -> exit
            putchar('\n');
            break;
        }
        // remove newline
        buf[strcspn(buf, "\r\n")] = '\0';
        trim(buf);
        if (buf[0] == '\0') continue;

        const char *resp = find_response(buf);
        printf("Bot: %s\n", resp);

        // if user asked to quit, break
        char lowbuf[LINE_BUF];
        strncpy(lowbuf, buf, sizeof(lowbuf)-1);
        lowbuf[sizeof(lowbuf)-1] = '\0';
        to_lower(lowbuf);
        if (strstr(lowbuf, "quit") || strstr(lowbuf, "exit") || strstr(lowbuf, "bye")) break;
    }

    printf("C Chatbot: Conversation ended.\n");
    return 0;
}
