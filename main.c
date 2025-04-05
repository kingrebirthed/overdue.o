/**
 * minimal-todo - A minimal ncurses-based todo TUI
 * Features: add/remove todos, categorize, search, filter, due dates, mark as done
 * License: GPL v3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#define MAX_TODOS 100
#define MAX_LENGTH 128
#define DATA_FILE ".todos.dat"

typedef struct {
    char text[MAX_LENGTH];
    char category[MAX_LENGTH];
    time_t due_date;
    int done;
} Todo;

Todo todos[MAX_TODOS];
int todo_count = 0;
int selected = 0;
char filter_category[MAX_LENGTH] = "";
int filter_done = -1; // -1: all, 0: pending, 1: done
char search_term[MAX_LENGTH] = "";

// Function prototypes
void init_screen();
void save_todos();
void load_todos();
void draw_screen();
void add_todo();
void delete_todo(int idx);
void toggle_todo(int idx);
void edit_todo(int idx);
void set_due_date(int idx);
void set_category(int idx);
void filter_by_category();
void filter_by_status();
void search_todos();
char* format_date(time_t date);
int todo_matches_filter(int idx);

int main() {
    init_screen();
    load_todos();

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'j': case KEY_DOWN:
                if (selected < todo_count - 1) selected++;
                break;
            case 'k': case KEY_UP:
                if (selected > 0) selected--;
                break;
            case 'a':
                add_todo();
                break;
            case 'd':
                if (todo_count > 0) delete_todo(selected);
                break;
            case ' ':
                if (todo_count > 0) toggle_todo(selected);
                break;
            case 'e':
                if (todo_count > 0) edit_todo(selected);
                break;
            case 'D':
                if (todo_count > 0) set_due_date(selected);
                break;
            case 'c':
                if (todo_count > 0) set_category(selected);
                break;
            case 'C':
                filter_by_category();
                break;
            case 'f':
                filter_by_status();
                break;
            case '/':
                search_todos();
                break;
            case 'r': // Reset filters
                filter_category[0] = '\0';
                filter_done = -1;
                search_term[0] = '\0';
                break;
        }
        draw_screen();
    }

    save_todos();
    endwin();
    return 0;
}

void init_screen() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Done
    init_pair(2, COLOR_RED, COLOR_BLACK);     // Overdue
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Due soon
    init_pair(4, COLOR_BLUE, COLOR_BLACK);    // Categories
}

void draw_screen() {
    clear();

    // Header
    attron(A_BOLD);
    mvprintw(0, 0, "MINIMAL TODO TUI");
    attroff(A_BOLD);

    mvprintw(1, 0, "Filter: %s | Status: %s | Search: %s",
             filter_category[0] ? filter_category : "All",
             filter_done == -1 ? "All" : (filter_done == 0 ? "Pending" : "Done"),
             search_term[0] ? search_term : "None");

    // Todos
    int y = 3;
    int visible_count = 0;

    for (int i = 0; i < todo_count; i++) {
        if (todo_matches_filter(i)) {
            // Highlight selected item
            if (i == selected) attron(A_REVERSE);

            // Done status
            mvprintw(y, 0, "[%c] ", todos[i].done ? 'X' : ' ');

            // Todo text with colors
            if (todos[i].done) {
                attron(COLOR_PAIR(1));
                mvprintw(y, 4, "%s", todos[i].text);
                attroff(COLOR_PAIR(1));
            } else {
                mvprintw(y, 4, "%s", todos[i].text);
            }

            // Category
            if (todos[i].category[0]) {
                attron(COLOR_PAIR(4));
                mvprintw(y, 4 + strlen(todos[i].text) + 1, "(%s)", todos[i].category);
                attroff(COLOR_PAIR(4));
            }

            // Due date
            if (todos[i].due_date > 0) {
                time_t now = time(NULL);
                time_t day_seconds = 24 * 60 * 60;

                if (todos[i].due_date < now) {
                    attron(COLOR_PAIR(2)); // Overdue
                } else if (todos[i].due_date < now + 2 * day_seconds) {
                    attron(COLOR_PAIR(3)); // Due soon
                }

                char* date_str = format_date(todos[i].due_date);
                mvprintw(y, COLS - strlen(date_str) - 1, "%s", date_str);
                free(date_str);

                attroff(COLOR_PAIR(2));
                attroff(COLOR_PAIR(3));
            }

            if (i == selected) attroff(A_REVERSE);
            y++;
            visible_count++;
        }
    }

    if (visible_count == 0) {
        mvprintw(y, 0, "No matching todos found.");
    }

    // Footer with commands
    mvprintw(LINES - 1, 0,
             "a:Add d:Delete Space:Toggle e:Edit D:Due c:Set-Cat C:Filter-Cat f:Filter-Status /:Search r:Reset q:Quit");

    refresh();
}

void add_todo() {
    echo();

    mvprintw(LINES - 2, 0, "New todo: ");
    clrtoeol();

    char new_todo[MAX_LENGTH];
    getstr(new_todo);

    if (new_todo[0] != '\0' && todo_count < MAX_TODOS) {
        strncpy(todos[todo_count].text, new_todo, MAX_LENGTH - 1);
        todos[todo_count].category[0] = '\0';
        todos[todo_count].due_date = 0;
        todos[todo_count].done = 0;
        todo_count++;
        selected = todo_count - 1;
    }

    noecho();
}

void delete_todo(int idx) {
    for (int i = idx; i < todo_count - 1; i++) {
        todos[i] = todos[i + 1];
    }
    todo_count--;
    if (selected >= todo_count && selected > 0) selected--;
}

void toggle_todo(int idx) {
    todos[idx].done = !todos[idx].done;
}

void edit_todo(int idx) {
    echo();

    mvprintw(LINES - 2, 0, "Edit todo: ");
    clrtoeol();

    char edit[MAX_LENGTH];
    getstr(edit);

    if (edit[0] != '\0') {
        strncpy(todos[idx].text, edit, MAX_LENGTH - 1);
    }

    noecho();
}

void set_due_date(int idx) {
    echo();

    mvprintw(LINES - 2, 0, "Due date (YYYY-MM-DD or blank to clear): ");
    clrtoeol();

    char date_str[MAX_LENGTH];
    getstr(date_str);

    if (date_str[0] == '\0') {
        todos[idx].due_date = 0;
    } else {
        struct tm tm_date = {0};
        if (sscanf(date_str, "%d-%d-%d", &tm_date.tm_year, &tm_date.tm_mon, &tm_date.tm_mday) == 3) {
            tm_date.tm_year -= 1900; // Years since 1900
            tm_date.tm_mon -= 1;     // Months are 0-11
            todos[idx].due_date = mktime(&tm_date);
        }
    }

    noecho();
}

void set_category(int idx) {
    echo();

    mvprintw(LINES - 2, 0, "Category: ");
    clrtoeol();

    char category[MAX_LENGTH];
    getstr(category);

    strncpy(todos[idx].category, category, MAX_LENGTH - 1);

    noecho();
}

void filter_by_category() {
    echo();

    mvprintw(LINES - 2, 0, "Filter by category (blank for all): ");
    clrtoeol();

    getstr(filter_category);
    selected = 0;

    noecho();
}

void filter_by_status() {
    filter_done = (filter_done + 1) % 3 - 1; // Cycle through -1, 0, 1
    selected = 0;
}

void search_todos() {
    echo();

    mvprintw(LINES - 2, 0, "Search: ");
    clrtoeol();

    getstr(search_term);
    selected = 0;

    noecho();
}

int todo_matches_filter(int idx) {
    // Status filter
    if (filter_done != -1 && todos[idx].done != filter_done) {
        return 0;
    }

    // Category filter
    if (filter_category[0] &&
        strcasecmp(todos[idx].category, filter_category) != 0) {
        return 0;
    }

    // Search filter
    if (search_term[0] &&
        strcasestr(todos[idx].text, search_term) == NULL &&
        strcasestr(todos[idx].category, search_term) == NULL) {
        return 0;
    }

    return 1;
}

char* format_date(time_t date) {
    struct tm *tm_info = localtime(&date);
    char *buffer = malloc(20);
    strftime(buffer, 20, "%Y-%m-%d", tm_info);
    return buffer;
}

void save_todos() {
    FILE *file = fopen(DATA_FILE, "wb");
    if (file) {
        fwrite(&todo_count, sizeof(int), 1, file);
        fwrite(todos, sizeof(Todo), todo_count, file);
        fclose(file);
    }
}

void load_todos() {
    FILE *file = fopen(DATA_FILE, "rb");
    if (file) {
        fread(&todo_count, sizeof(int), 1, file);
        fread(todos, sizeof(Todo), todo_count, file);
        fclose(file);
    }
}
