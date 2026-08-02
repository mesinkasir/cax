#include "markdown.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void parse_inline_formatting(const char *input, char *output, size_t out_size) {
    size_t in_len = strlen(input);
    size_t out_idx = 0;
    size_t i = 0;

    while (i < in_len && out_idx < out_size - 1) {
        if (input[i] == '`') {
            const char *end = strchr(input + i + 1, '`');
            if (end) {
                size_t len = end - (input + i + 1);
                out_idx += snprintf(output + out_idx, out_size - out_idx, "<code>%.*s</code>", (int)len, input + i + 1);
                i = (end - input) + 1;
                continue;
            }
        }

        if (input[i] == '!' && i + 1 < in_len && input[i + 1] == '[') {
            const char *close_bracket = strchr(input + i + 2, ']');
            if (close_bracket && *(close_bracket + 1) == '(') {
                const char *close_paren = strchr(close_bracket + 2, ')');
                if (close_paren) {
                    size_t alt_len = close_bracket - (input + i + 2);
                    size_t src_len = close_paren - (close_bracket + 2);
                    out_idx += snprintf(output + out_idx, out_size - out_idx,
                                        "<img src=\"%.*s\" alt=\"%.*s\" />",
                                        (int)src_len, close_bracket + 2,
                                        (int)alt_len, input + i + 2);
                    i = (close_paren - input) + 1;
                    continue;
                }
            }
        }

        if (input[i] == '[') {
            const char *close_bracket = strchr(input + i + 1, ']');
            if (close_bracket && *(close_bracket + 1) == '(') {
                const char *close_paren = strchr(close_bracket + 2, ')');
                if (close_paren) {
                    size_t text_len = close_bracket - (input + i + 1);
                    size_t url_len = close_paren - (close_bracket + 2);

                    char text_buf[512] = {0};
                    snprintf(text_buf, sizeof(text_buf), "%.*s", (int)text_len, input + i + 1);

                    char parsed_text[512] = {0};
                    parse_inline_formatting(text_buf, parsed_text, sizeof(parsed_text));

                    out_idx += snprintf(output + out_idx, out_size - out_idx,
                                        "<a href=\"%.*s\">%s</a>",
                                        (int)url_len, close_bracket + 2, parsed_text);
                    i = (close_paren - input) + 1;
                    continue;
                }
            }
        }

        if (i + 1 < in_len && input[i] == '*' && input[i + 1] == '*') {
            const char *end = strstr(input + i + 2, "**");
            if (end) {
                size_t len = end - (input + i + 2);
                out_idx += snprintf(output + out_idx, out_size - out_idx, "<strong>%.*s</strong>", (int)len, input + i + 2);
                i = (end - input) + 2;
                continue;
            }
        }

        if (i + 1 < in_len && input[i] == '~' && input[i + 1] == '~') {
            const char *end = strstr(input + i + 2, "~~");
            if (end) {
                size_t len = end - (input + i + 2);
                out_idx += snprintf(output + out_idx, out_size - out_idx, "<del>%.*s</del>", (int)len, input + i + 2);
                i = (end - input) + 2;
                continue;
            }
        }

        if (input[i] == '*' || input[i] == '_') {
            char target = input[i];
            const char *end = strchr(input + i + 1, target);
            if (end && (end == input + i + 1 || *(end - 1) != target)) {
                size_t len = end - (input + i + 1);
                out_idx += snprintf(output + out_idx, out_size - out_idx, "<i>%.*s</i>", (int)len, input + i + 1);
                i = (end - input) + 1;
                continue;
            }
        }

        output[out_idx++] = input[i++];
    }
    output[out_idx] = '\0';
}

static void render_table_row(const char *line, char *output, size_t out_size, const char *tag) {
    char temp[4096];
    strncpy(temp, line, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    snprintf(output, out_size, "<tr>");

    char *token = strtok(temp, "|");
    while (token) {
        while (isspace((unsigned char)*token)) token++;
        char *end = token + strlen(token) - 1;
        while (end > token && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }

        if (strlen(token) > 0) {
            char cell_out[2048];
            parse_inline_formatting(token, cell_out, sizeof(cell_out));
            char cell_buf[2048];
            snprintf(cell_buf, sizeof(cell_buf), "<%s>%s</%s>", tag, cell_out, tag);
            strncat(output, cell_buf, out_size - strlen(output) - 1);
        }
        token = strtok(NULL, "|");
    }
    strncat(output, "</tr>\n", out_size - strlen(output) - 1);
}

static int is_table_divider(const char *line) {
    if (!strchr(line, '|')) return 0;
    for (size_t i = 0; line[i] != '\0'; i++) {
        if (line[i] != '|' && line[i] != '-' && line[i] != ':' && !isspace((unsigned char)line[i])) {
            return 0;
        }
    }
    return 1;
}

char *cax_markdown_to_html(const char *md_content) {
    if (!md_content) return NULL;

    size_t buf_size = strlen(md_content) * 4 + 4096;
    char *html_output = malloc(buf_size);
    if (!html_output) return NULL;
    html_output[0] = '\0';

    char *work_copy = cax_strdup(md_content);
    if (!work_copy) {
        free(html_output);
        return NULL;
    }

    char *cursor = work_copy;

    int in_ul = 0;
    int in_ol = 0;
    int in_table = 0;
    int in_code = 0;

    while (*cursor) {
        char *line_start = cursor;
        while (*cursor && *cursor != '\n') {
            cursor++;
        }

        if (*cursor == '\n') {
            *cursor = '\0';
            cursor++;
        }

        size_t line_len = strlen(line_start);
        if (line_len > 0 && line_start[line_len - 1] == '\r') {
            line_start[line_len - 1] = '\0';
        }

        if (strncmp(line_start, "```", 3) == 0) {
            if (in_code) {
                strncat(html_output, "</code></pre>\n", buf_size - strlen(html_output) - 1);
                in_code = 0;
            } else {
                if (in_ul) { strncat(html_output, "</ul>\n", buf_size - strlen(html_output) - 1); in_ul = 0; }
                if (in_ol) { strncat(html_output, "</ol>\n", buf_size - strlen(html_output) - 1); in_ol = 0; }
                if (in_table) { strncat(html_output, "</tbody></table>\n", buf_size - strlen(html_output) - 1); in_table = 0; }
                strncat(html_output, "<pre><code>", buf_size - strlen(html_output) - 1);
                in_code = 1;
            }
            continue;
        }

        if (in_code) {
            strncat(html_output, line_start, buf_size - strlen(html_output) - 1);
            strncat(html_output, "\n", buf_size - strlen(html_output) - 1);
            continue;
        }

        if (is_table_divider(line_start)) {
            continue;
        }

        if (line_start[0] == '|' || strchr(line_start, '|') != NULL) {
            if (in_ul) { strncat(html_output, "</ul>\n", buf_size - strlen(html_output) - 1); in_ul = 0; }
            if (in_ol) { strncat(html_output, "</ol>\n", buf_size - strlen(html_output) - 1); in_ol = 0; }

            char row_buf[4096] = {0};
            if (!in_table) {
                strncat(html_output, "<table><thead>\n", buf_size - strlen(html_output) - 1);
                render_table_row(line_start, row_buf, sizeof(row_buf), "th");
                strncat(html_output, row_buf, buf_size - strlen(html_output) - 1);
                strncat(html_output, "</thead><tbody>\n", buf_size - strlen(html_output) - 1);
                in_table = 1;
            } else {
                render_table_row(line_start, row_buf, sizeof(row_buf), "td");
                strncat(html_output, row_buf, buf_size - strlen(html_output) - 1);
            }
            continue;
        } else if (in_table) {
            strncat(html_output, "</tbody></table>\n", buf_size - strlen(html_output) - 1);
            in_table = 0;
        }

        if (strlen(line_start) == 0) {
            if (in_ul) { strncat(html_output, "</ul>\n", buf_size - strlen(html_output) - 1); in_ul = 0; }
            if (in_ol) { strncat(html_output, "</ol>\n", buf_size - strlen(html_output) - 1); in_ol = 0; }
            continue;
        }

        if ((strncmp(line_start, "* ", 2) == 0) || (strncmp(line_start, "- ", 2) == 0)) {
            if (in_ol) { strncat(html_output, "</ol>\n", buf_size - strlen(html_output) - 1); in_ol = 0; }
            if (!in_ul) {
                strncat(html_output, "<ul>\n", buf_size - strlen(html_output) - 1);
                in_ul = 1;
            }
            char inline_buf[2048];
            parse_inline_formatting(line_start + 2, inline_buf, sizeof(inline_buf));
            char line_buf[4096];
            snprintf(line_buf, sizeof(line_buf), "<li>%s</li>\n", inline_buf);
            strncat(html_output, line_buf, buf_size - strlen(html_output) - 1);
            continue;
        } else if (in_ul) {
            strncat(html_output, "</ul>\n", buf_size - strlen(html_output) - 1);
            in_ul = 0;
        }

        if (isdigit((unsigned char)line_start[0]) && strncmp(line_start + 1, ". ", 2) == 0) {
            if (in_ul) { strncat(html_output, "</ul>\n", buf_size - strlen(html_output) - 1); in_ul = 0; }
            if (!in_ol) {
                strncat(html_output, "<ol>\n", buf_size - strlen(html_output) - 1);
                in_ol = 1;
            }
            char inline_buf[2048];
            parse_inline_formatting(line_start + 3, inline_buf, sizeof(inline_buf));
            char line_buf[4096];
            snprintf(line_buf, sizeof(line_buf), "<li>%s</li>\n", inline_buf);
            strncat(html_output, line_buf, buf_size - strlen(html_output) - 1);
            continue;
        } else if (in_ol) {
            strncat(html_output, "</ol>\n", buf_size - strlen(html_output) - 1);
            in_ol = 0;
        }

        if (strncmp(line_start, "> ", 2) == 0) {
            char inline_buf[2048];
            parse_inline_formatting(line_start + 2, inline_buf, sizeof(inline_buf));
            char line_buf[4096];
            snprintf(line_buf, sizeof(line_buf), "<blockquote><p>%s</p></blockquote>\n", inline_buf);
            strncat(html_output, line_buf, buf_size - strlen(html_output) - 1);
            continue;
        }

        char inline_buf[2048];
        char final_line[4096];

        if (strncmp(line_start, "###### ", 7) == 0) {
            parse_inline_formatting(line_start + 7, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<h6>%s</h6>\n", inline_buf);
        } else if (strncmp(line_start, "##### ", 6) == 0) {
            parse_inline_formatting(line_start + 6, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<h5>%s</h5>\n", inline_buf);
        } else if (strncmp(line_start, "#### ", 5) == 0) {
            parse_inline_formatting(line_start + 5, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<h4>%s</h4>\n", inline_buf);
        } else if (strncmp(line_start, "### ", 4) == 0) {
            parse_inline_formatting(line_start + 4, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<h3>%s</h3>\n", inline_buf);
        } else if (strncmp(line_start, "## ", 3) == 0) {
            parse_inline_formatting(line_start + 3, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<h2>%s</h2>\n", inline_buf);
        } else if (strncmp(line_start, "# ", 2) == 0) {
            parse_inline_formatting(line_start + 2, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<h1>%s</h1>\n", inline_buf);
        } else {
            parse_inline_formatting(line_start, inline_buf, sizeof(inline_buf));
            snprintf(final_line, sizeof(final_line), "<p>%s</p>\n", inline_buf);
        }

        strncat(html_output, final_line, buf_size - strlen(html_output) - 1);
    }

    if (in_ul) strncat(html_output, "</ul>\n", buf_size - strlen(html_output) - 1);
    if (in_ol) strncat(html_output, "</ol>\n", buf_size - strlen(html_output) - 1);
    if (in_table) strncat(html_output, "</tbody></table>\n", buf_size - strlen(html_output) - 1);
    if (in_code) strncat(html_output, "</code></pre>\n", buf_size - strlen(html_output) - 1);

    free(work_copy);
    return html_output;
}