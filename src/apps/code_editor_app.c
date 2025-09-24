#include "app.h"
#include "../display.h"
#include "../keys.h"
#include "../board_pins.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FLASH_TARGET_OFFSET (1536 * 1024) // 1.5MB offset into flash
#define MAX_FILE_SIZE 16384
#define MAX_FILES 8
#define FILE_METADATA_SIZE 256

#define MAX_LINE_LENGTH 40
#define MAX_LINES 500
#define VISIBLE_LINES 8
#define TAB_SIZE 2

typedef struct {
    char filename[32];
    char* buffer;
    int buffer_size;
    int cursor_x;
    int cursor_y;
    int scroll_y;
    int modified;
    int mode;  // 0=edit, 1=file browser, 2=settings
    char status_message[64];
    char* lines[MAX_LINES];
    int line_count;
    int current_file_index;
    char file_list[MAX_FILES][32];
    int file_count;
    int selected_file;
    int syntax_highlighting;  // 0=off, 1=Python, 2=C
} code_editor_data_t;

static app_t code_editor_app;
static code_editor_data_t code_editor_data;

static void code_editor_init(app_t* app);
static void code_editor_update(app_t* app);
static void code_editor_render(app_t* app);
static void code_editor_process_key(app_t* app, uint8_t key_code);
static void code_editor_destroy(app_t* app);

static void load_file(code_editor_data_t* data);
static void save_file(code_editor_data_t* data);
static void new_file(code_editor_data_t* data);
static void load_file_list(code_editor_data_t* data);
static void insert_char(code_editor_data_t* data, char c);
static void delete_char(code_editor_data_t* data);
static void move_cursor(code_editor_data_t* data, int dx, int dy);
static void ensure_cursor_visible(code_editor_data_t* data);
static void parse_buffer_into_lines(code_editor_data_t* data);
static uint16_t get_syntax_color(const char* text, int pos, int syntax_mode);
static void render_editor(code_editor_data_t* data);
static void render_file_browser(code_editor_data_t* data);
static void render_settings(code_editor_data_t* data);
static void show_message(code_editor_data_t* data, const char* msg);

app_t* register_code_editor_app(void) {
    code_editor_app.name = "Code Editor";
    code_editor_app.icon = "C";
    code_editor_app.app_data = &code_editor_data;
    code_editor_app.init = code_editor_init;
    code_editor_app.update = code_editor_update;
    code_editor_app.render = code_editor_render;
    code_editor_app.process_key = code_editor_process_key;
    code_editor_app.destroy = code_editor_destroy;
    
    return &code_editor_app;
}

static void code_editor_init(app_t* app) {
    code_editor_data_t* data = (code_editor_data_t*)app->app_data;
    
    if (data->buffer == NULL) {
        data->buffer = malloc(MAX_FILE_SIZE);
    }
    
    if (data->buffer == NULL) {
        strcpy(data->status_message, "Error: Memory allocation failed");
        return;
    }
    
    strcpy(data->filename, "untitled.py");
    data->buffer_size = 0;
    data->cursor_x = 0;
    data->cursor_y = 0;
    data->scroll_y = 0;
    data->modified = 0;
    data->mode = 1; // Start with file browser
    data->syntax_highlighting = 1; // Python syntax highlighting
    strcpy(data->status_message, "Welcome to Code Editor");
    
    // Initialize lines
    for (int i = 0; i < MAX_LINES; i++) {
        data->lines[i] = NULL;
    }
    data->line_count = 0;
    
    // Load file list
    load_file_list(data);
    data->selected_file = 0;
    data->current_file_index = -1;
    
    // Create an empty buffer with a single line
    strcpy(data->buffer, "");
    data->buffer_size = 0;
    parse_buffer_into_lines(data);
}

static void code_editor_update(app_t* app) {
    // Nothing to update automatically
}

static void code_editor_render(app_t* app) {
    code_editor_data_t* data = (code_editor_data_t*)app->app_data;
    
    // Draw header
    display_fill_rect(0, 0, DISPLAY_WIDTH, 20, COLOR_DARKGRAY);
    
    switch (data->mode) {
        case 0: // Editor
            display_draw_text_aligned(DISPLAY_WIDTH / 2, 10, data->filename, FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
            render_editor(data);
            break;
            
        case 1: // File browser
            display_draw_text_aligned(DISPLAY_WIDTH / 2, 10, "File Browser", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
            render_file_browser(data);
            break;
            
        case 2: // Settings
            display_draw_text_aligned(DISPLAY_WIDTH / 2, 10, "Editor Settings", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
            render_settings(data);
            break;
    }
    
    // Draw status bar
    display_fill_rect(0, DISPLAY_HEIGHT - 20, DISPLAY_WIDTH, 20, COLOR_DARKGRAY);
    display_draw_text(5, DISPLAY_HEIGHT - 15, data->status_message, FONT_SMALL, COLOR_WHITE);
    
    // Draw footer with controls
    display_fill_rect(0, DISPLAY_HEIGHT - 40, DISPLAY_WIDTH, 20, COLOR_BLUE);
    
    switch (data->mode) {
        case 0: // Editor
            display_draw_text(5, DISPLAY_HEIGHT - 35, "1:Save 2:Up 8:Down 4:Left 6:Right 0:Menu", FONT_SMALL, COLOR_WHITE);
            break;
            
        case 1: // File browser
            display_draw_text(5, DISPLAY_HEIGHT - 35, "2:Up 8:Down 5:Open 6:New 0:Back", FONT_SMALL, COLOR_WHITE);
            break;
            
        case 2: // Settings
            display_draw_text(5, DISPLAY_HEIGHT - 35, "2:Up 8:Down 5:Select 0:Back", FONT_SMALL, COLOR_WHITE);
            break;
    }
}

static void code_editor_process_key(app_t* app, uint8_t key_code) {
    code_editor_data_t* data = (code_editor_data_t*)app->app_data;
    
    switch (data->mode) {
        case 0: // Editor mode
            switch (key_code) {
                case KEY_0: // Menu
                    data->mode = 1; // Switch to file browser
                    break;
                    
                case KEY_1: // Save
                    save_file(data);
                    break;
                    
                case KEY_2: // Cursor up
                    move_cursor(data, 0, -1);
                    break;
                    
                case KEY_8: // Cursor down
                    move_cursor(data, 0, 1);
                    break;
                    
                case KEY_4: // Cursor left
                    move_cursor(data, -1, 0);
                    break;
                    
                case KEY_6: // Cursor right
                    move_cursor(data, 1, 0);
                    break;
                    
                case KEY_5: // Insert space
                    insert_char(data, ' ');
                    break;
                    
                case KEY_CLEAR: // Delete
                    delete_char(data);
                    break;
                    
                default:
                    // Insert digit characters
                    if (key_code >= KEY_0 && key_code <= KEY_9) {
                        insert_char(data, '0' + key_code);
                    }
                    break;
            }
            break;
            
        case 1: // File browser mode
            switch (key_code) {
                case KEY_0: // Back to launcher
                    app_manager_return_to_launcher();
                    break;
                    
                case KEY_2: // Up
                    if (data->selected_file > 0) {
                        data->selected_file--;
                    }
                    break;
                    
                case KEY_8: // Down
                    if (data->selected_file < data->file_count - 1) {
                        data->selected_file++;
                    }
                    break;
                    
                case KEY_5: // Open selected file
                    if (data->file_count > 0) {
                        strcpy(data->filename, data->file_list[data->selected_file]);
                        data->current_file_index = data->selected_file;
                        load_file(data);
                        data->mode = 0; // Switch to editor
                    }
                    break;
                    
                case KEY_6: // New file
                    new_file(data);
                    data->mode = 0; // Switch to editor
                    break;
                    
                case KEY_7: // Settings
                    data->mode = 2; // Switch to settings
                    break;
                    
                default:
                    break;
            }
            break;
            
        case 2: // Settings mode
            switch (key_code) {
                case KEY_0: // Back to file browser
                    data->mode = 1;
                    break;
                    
                case KEY_2: // Up
                    // Navigate settings
                    break;
                    
                case KEY_8: // Down
                    // Navigate settings
                    break;
                    
                case KEY_5: // Toggle/select setting
                    // Toggle syntax highlighting
                    data->syntax_highlighting = (data->syntax_highlighting + 1) % 3;
                    break;
                    
                default:
                    break;
            }
            break;
    }
}

static void code_editor_destroy(app_t* app) {
    code_editor_data_t* data = (code_editor_data_t*)app->app_data;
    
    // Free memory
    if (data->buffer != NULL) {
        free(data->buffer);
        data->buffer = NULL;
    }
    
    // Free line pointers
    for (int i = 0; i < data->line_count; i++) {
        if (data->lines[i] != NULL) {
            free(data->lines[i]);
            data->lines[i] = NULL;
        }
    }
}

static void load_file(code_editor_data_t* data) {
    // In a real implementation, this would load from flash
    // For this example, we'll create a sample file
    
    if (strcmp(data->filename, "example.py") == 0) {
        const char* sample = "# Example Python code\n\ndef hello_world():\n  print(\"Hello, PicoCalc!\")\n  return 42\n\n# Call the function\nresult = hello_world()\n";
        strncpy(data->buffer, sample, MAX_FILE_SIZE - 1);
        data->buffer_size = strlen(sample);
    } else if (strcmp(data->filename, "test.c") == 0) {
        const char* sample = "#include <stdio.h>\n\nint main() {\n  printf(\"Hello, PicoCalc!\\n\");\n  return 0;\n}\n";
        strncpy(data->buffer, sample, MAX_FILE_SIZE - 1);
        data->buffer_size = strlen(sample);
    } else {
        // New or unknown file
        strcpy(data->buffer, "");
        data->buffer_size = 0;
    }
    
    // Parse buffer into lines
    parse_buffer_into_lines(data);
    
    // Reset cursor and scroll
    data->cursor_x = 0;
    data->cursor_y = 0;
    data->scroll_y = 0;
    data->modified = 0;
    
    show_message(data, "File loaded");
}

static void save_file(code_editor_data_t* data) {
    // In a real implementation, this would save to flash
    // For this example, we'll just show a message
    
    show_message(data, "File saved");
    data->modified = 0;
    
    // Add to file list if it's a new file
    if (data->current_file_index == -1) {
        if (data->file_count < MAX_FILES) {
            strcpy(data->file_list[data->file_count], data->filename);
            data->current_file_index = data->file_count;
            data->file_count++;
        }
    }
}

static void new_file(code_editor_data_t* data) {
    // Create a new file
    strcpy(data->filename, "untitled.py");
    strcpy(data->buffer, "");
    data->buffer_size = 0;
    
    // Parse buffer into lines
    parse_buffer_into_lines(data);
    
    // Reset cursor and scroll
    data->cursor_x = 0;
    data->cursor_y = 0;
    data->scroll_y = 0;
    data->modified = 0;
    data->current_file_index = -1;
    
    show_message(data, "New file created");
}

static void load_file_list(code_editor_data_t* data) {
    // In a real implementation, this would scan flash storage
    // For this example, we'll create some sample files
    
    data->file_count = 2;
    strcpy(data->file_list[0], "example.py");
    strcpy(data->file_list[1], "test.c");
}

static void insert_char(code_editor_data_t* data, char c) {
    if (data->buffer_size >= MAX_FILE_SIZE - 1) {
        show_message(data, "Error: File too large");
        return;
    }
    
    // Find position in buffer
    int pos = 0;
    for (int i = 0; i < data->cursor_y; i++) {
        pos += strlen(data->lines[i]) + 1; // +1 for newline
    }
    pos += data->cursor_x;
    
    // Insert character
    memmove(data->buffer + pos + 1, data->buffer + pos, data->buffer_size - pos + 1);
    data->buffer[pos] = c;
    data->buffer_size++;
    
    // Update cursor
    data->cursor_x++;
    
    // Re-parse lines
    parse_buffer_into_lines(data);
    
    data->modified = 1;
}

static void delete_char(code_editor_data_t* data) {
    // Find position in buffer
    int pos = 0;
    for (int i = 0; i < data->cursor_y; i++) {
        pos += strlen(data->lines[i]) + 1; // +1 for newline
    }
    pos += data->cursor_x;
    
    if (pos > 0) {
        // Delete character before cursor
        memmove(data->buffer + pos - 1, data->buffer + pos, data->buffer_size - pos + 1);
        data->buffer_size--;
        
        // Update cursor
        if (data->cursor_x > 0) {
            data->cursor_x--;
        } else if (data->cursor_y > 0) {
            data->cursor_y--;
            data->cursor_x = strlen(data->lines[data->cursor_y]);
        }
        
        // Re-parse lines
        parse_buffer_into_lines(data);
        
        data->modified = 1;
    }
}

static void move_cursor(code_editor_data_t* data, int dx, int dy) {
    // Move horizontally
    data->cursor_x += dx;
    if (data->cursor_x < 0) {
        if (data->cursor_y > 0) {
            data->cursor_y--;
            data->cursor_x = strlen(data->lines[data->cursor_y]);
        } else {
            data->cursor_x = 0;
        }
    } else if (data->cursor_y < data->line_count && data->cursor_x > strlen(data->lines[data->cursor_y])) {
        if (data->cursor_y < data->line_count - 1) {
            data->cursor_y++;
            data->cursor_x = 0;
        } else {
            data->cursor_x = strlen(data->lines[data->cursor_y]);
        }
    }
    
    // Move vertically
    data->cursor_y += dy;
    if (data->cursor_y < 0) {
        data->cursor_y = 0;
    } else if (data->cursor_y >= data->line_count) {
        data->cursor_y = data->line_count - 1;
    }
    
    // Ensure cursor is not beyond end of line
    if (data->cursor_y < data->line_count && data->cursor_x > strlen(data->lines[data->cursor_y])) {
        data->cursor_x = strlen(data->lines[data->cursor_y]);
    }
    
    // Ensure cursor is visible
    ensure_cursor_visible(data);
}

static void ensure_cursor_visible(code_editor_data_t* data) {
    // Adjust scroll position to make cursor visible
    if (data->cursor_y < data->scroll_y) {
        data->scroll_y = data->cursor_y;
    } else if (data->cursor_y >= data->scroll_y + VISIBLE_LINES) {
        data->scroll_y = data->cursor_y - VISIBLE_LINES + 1;
    }
    
    // Ensure scroll position is valid
    if (data->scroll_y < 0) {
        data->scroll_y = 0;
    }
}

static void parse_buffer_into_lines(code_editor_data_t* data) {
    // Free existing lines
    for (int i = 0; i < data->line_count; i++) {
        if (data->lines[i] != NULL) {
            free(data->lines[i]);
            data->lines[i] = NULL;
        }
    }
    
    // Parse buffer into lines
    data->line_count = 0;
    char* line_start = data->buffer;
    char* line_end = strchr(line_start, '\n');
    
    while (line_start < data->buffer + data->buffer_size && data->line_count < MAX_LINES) {
        int line_length;
        
        if (line_end != NULL) {
            line_length = line_end - line_start;
            line_end++; // Skip newline
        } else {
            line_length = strlen(line_start);
            line_end = line_start + line_length;
        }
        
        // Allocate and copy line
        data->lines[data->line_count] = malloc(line_length + 1);
        if (data->lines[data->line_count] != NULL) {
            strncpy(data->lines[data->line_count], line_start, line_length);
            data->lines[data->line_count][line_length] = '\0';
            data->line_count++;
        }
        
        // Move to next line
        line_start = line_end;
        line_end = strchr(line_start, '\n');
    }
    
    // Ensure at least one line exists
    if (data->line_count == 0) {
        data->lines[0] = malloc(1);
        if (data->lines[0] != NULL) {
            data->lines[0][0] = '\0';
            data->line_count = 1;
        }
    }
}

static uint16_t get_syntax_color(const char* text, int pos, int syntax_mode) {
    // Simple syntax highlighting
    if (syntax_mode == 0) {
        return COLOR_WHITE; // No highlighting
    }
    
    // Check for comments
    if (text[0] == '#' || (pos >= 2 && text[pos-2] == '/' && text[pos-1] == '/')) {
        return COLOR_GREEN;
    }
    
    // Check for keywords
    const char* keywords_py[] = {"def", "class", "if", "else", "elif", "for", "while", "import", "from", "return", "True", "False", "None"};
    const char* keywords_c[] = {"int", "char", "void", "return", "if", "else", "for", "while", "struct", "typedef", "const", "static"};
    
    const char** keywords;
    int keyword_count;
    
    if (syntax_mode == 1) { // Python
        keywords = keywords_py;
        keyword_count = sizeof(keywords_py) / sizeof(keywords_py[0]);
    } else { // C
        keywords = keywords_c;
        keyword_count = sizeof(keywords_c) / sizeof(keywords_c[0]);
    }
    
    for (int i = 0; i < keyword_count; i++) {
        if (strstr(text, keywords[i]) != NULL) {
            return COLOR_BLUE;
        }
    }
    
    // Check for strings
    if ((text[pos] == '"' || text[pos] == '\'') || 
        (pos > 0 && (text[pos-1] == '"' || text[pos-1] == '\''))) {
        return COLOR_YELLOW;
    }
    
    // Check for numbers
    if (text[pos] >= '0' && text[pos] <= '9') {
        return COLOR_MAGENTA;
    }
    
    return COLOR_WHITE;
}

static void render_editor(code_editor_data_t* data) {
    // Draw editor area
    display_fill_rect(0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT - 60, COLOR_BLACK);
    
    // Draw line numbers and text
    for (int i = 0; i < VISIBLE_LINES && i + data->scroll_y < data->line_count; i++) {
        int y = 30 + i * 20;
        char line_num[8];
        sprintf(line_num, "%3d", i + data->scroll_y + 1);
        
        // Line number
        display_draw_text(5, y, line_num, FONT_SMALL, COLOR_GRAY);
        
        // Line text with syntax highlighting
        const char* line_text = data->lines[i + data->scroll_y];
        int x = 30;
        
        if (data->syntax_highlighting > 0) {
            // With syntax highlighting
            for (int j = 0; j < strlen(line_text); j++) {
                char c[2] = {line_text[j], '\0'};
                uint16_t color = get_syntax_color(line_text, j, data->syntax_highlighting);
                display_draw_text(x, y, c, FONT_SMALL, color);
                x += display_get_text_width(c, FONT_SMALL);
            }
        } else {
            // Without syntax highlighting
            display_draw_text(30, y, line_text, FONT_SMALL, COLOR_WHITE);
        }
        
        // Draw cursor
        if (i + data->scroll_y == data->cursor_y) {
            int cursor_x = 30;
            for (int j = 0; j < data->cursor_x && j < strlen(line_text); j++) {
                char c[2] = {line_text[j], '\0'};
                cursor_x += display_get_text_width(c, FONT_SMALL);
            }
            display_fill_rect(cursor_x, y - 2, 2, 12, COLOR_WHITE);
        }
    }
    
    // Draw scroll indicators if needed
    if (data->scroll_y > 0) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, 25, "▲", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    }
    if (data->scroll_y + VISIBLE_LINES < data->line_count) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 45, "▼", FONT_SMALL, ALIGN_CENTER, COLOR_WHITE);
    }
}

static void render_file_browser(code_editor_data_t* data) {
    // Draw file browser area
    display_fill_rect(0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT - 60, COLOR_BLACK);
    
    // Draw file list
    if (data->file_count == 0) {
        display_draw_text_aligned(DISPLAY_WIDTH / 2, 100, "No files found", FONT_MEDIUM, ALIGN_CENTER, COLOR_WHITE);
    } else {
        for (int i = 0; i < data->file_count; i++) {
            int y = 40 + i * 25;
            
            // Highlight selected file
            if (i == data->selected_file) {
                display_fill_rect(10, y - 5, DISPLAY_WIDTH - 20, 20, COLOR_BLUE);
            }
            
            // Draw file icon based on extension
            const char* ext = strrchr(data->file_list[i], '.');
            if (ext != NULL && strcmp(ext, ".py") == 0) {
                display_draw_text(20, y, "[PY]", FONT_SMALL, COLOR_YELLOW);
            } else if (ext != NULL && strcmp(ext, ".c") == 0) {
                display_draw_text(20, y, "[C]", FONT_SMALL, COLOR_GREEN);
            } else {
                display_draw_text(20, y, "[TXT]", FONT_SMALL, COLOR_WHITE);
            }
            
            // Draw filename
            display_draw_text(60, y, data->file_list[i], FONT_SMALL, COLOR_WHITE);
        }
    }
    
    // Draw "New File" option
    display_fill_rect(10, 40 + data->file_count * 25, DISPLAY_WIDTH - 20, 20, 
                     data->selected_file == data->file_count ? COLOR_GREEN : COLOR_DARKGRAY);
    display_draw_text(20, 40 + data->file_count * 25 + 5, "Create New File...", FONT_SMALL, COLOR_WHITE);
}

static void render_settings(code_editor_data_t* data) {
    // Draw settings area
    display_fill_rect(0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT - 60, COLOR_BLACK);
    
    // Draw settings options
    display_draw_text(20, 40, "Syntax Highlighting:", FONT_SMALL, COLOR_WHITE);
    
    const char* syntax_options[] = {"Off", "Python", "C/C++"};
    display_draw_text(180, 40, syntax_options[data->syntax_highlighting], FONT_SMALL, COLOR_GREEN);
    
    // Draw other settings
    display_draw_text(20, 70, "Tab Size:", FONT_SMALL, COLOR_WHITE);
    char tab_str[8];
    sprintf(tab_str, "%d spaces", TAB_SIZE);
    display_draw_text(180, 70, tab_str, FONT_SMALL, COLOR_WHITE);
}

static void show_message(code_editor_data_t* data, const char* msg) {
    strcpy(data->status_message, msg);
}
