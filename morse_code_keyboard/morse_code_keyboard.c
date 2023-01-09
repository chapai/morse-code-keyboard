#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <unistd.h>

// morse code tree node
struct MorseTreeNode {
    char value;
    char* lable;
    struct MorseTreeNode* left; // dot
    struct MorseTreeNode* right; // dash
};

struct MorseCodeModel {
    struct MorseTreeNode* tree_root;
    struct MorseTreeNode* current_symbol;
};

struct MorseCodeApp {
    struct MorseCodeModel* model;
    struct FuriMutex* model_mutex;

    struct FuriMessageQueue* event_queue;
    struct ViewPort* view_port;
    struct Gui* gui;
};

struct MorseTreeNode* create_a_node(char value, char* lable) {
    struct MorseTreeNode* node = malloc(sizeof(struct MorseTreeNode));

    node->value = value;
    node->lable = lable;
    node->left = NULL;
    node->right = NULL;

    return node;
}

// TODO: 
//  - finish the tree
//  - map keyboard code to eache node
//  - add special characters, like enter, space, etc
struct MorseTreeNode* build_morse_tree() {
    // root
    struct MorseTreeNode* root = create_a_node('\0', "Start");

    // first layer
    root->left = create_a_node('e', "Letter E");
    root->right = create_a_node('t', "Letter T");

    // second layer
    root->left->left = create_a_node('i', "Letter I");
    root->left->right = create_a_node('a', "Letter A");
    root->right->left = create_a_node('n', "Letter N");
    root->right->right = create_a_node('m', "Letter M");

    // third layer

    return root;
}

void free_tree(struct MorseTreeNode* root) {
    if(root == NULL) return;

    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

void draw_callback(struct Canvas* canvas, void* ctx) {
    UNUSED(canvas);

    struct MorseCodeApp* context = ctx;
    furi_check(furi_mutex_acquire(context->model_mutex, FuriWaitForever) == FuriStatusOk);

    // UI begin

    canvas_draw_frame(canvas, 0, 0, 128, 64);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 12, "Volume 10");

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 66, 12, "Speed 400");

    canvas_draw_frame(canvas, 0, 0, 128, 17);

    canvas_draw_frame(canvas, 0, 16, 128, 17);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 28, "Current");

    canvas_draw_str(canvas, 46, 28, context->model->current_symbol->lable);

    // UI end

    furi_mutex_release(context->model_mutex);
}

void input_callback(InputEvent* input, void* ctx) {
    struct MorseCodeApp* context = (struct MorseCodeApp*)ctx;

    furi_message_queue_put(context->event_queue, input, 0);
}

struct MorseCodeApp* morse_code_app_alloc() {
    struct MorseCodeApp* instance = malloc(sizeof(struct MorseCodeApp));

    instance->model = malloc(sizeof(struct MorseCodeModel));
    instance->model->tree_root = build_morse_tree();
    instance->model->current_symbol = instance->model->tree_root;

    instance->model_mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    instance->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    instance->view_port = view_port_alloc();
    view_port_draw_callback_set(instance->view_port, draw_callback, instance);
    view_port_input_callback_set(instance->view_port, input_callback, instance);

    instance->gui = (struct Gui*)furi_record_open("gui");
    gui_add_view_port(instance->gui, instance->view_port, GuiLayerFullscreen);

    return instance;
}

void morse_code_app_free(struct MorseCodeApp* instance) {
    view_port_enabled_set(instance->view_port, false); // Disabsles our ViewPort
    gui_remove_view_port(instance->gui, instance->view_port); // Removes our ViewPort from the Gui
    furi_record_close("gui"); // Closes the gui record
    view_port_free(instance->view_port); // Frees memory allocated by view_port_alloc
    furi_message_queue_free(instance->event_queue);

    furi_mutex_free(instance->model_mutex);

    free_tree(instance->model->tree_root);
    free(instance->model);
    free(instance);
}

int32_t morse_code_keyboard(void* p) {
    UNUSED(p);

    struct MorseCodeApp* app_context = morse_code_app_alloc();

    //set up usb hid
    FuriHalUsbInterface* usb_mode_prev = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid, NULL) == true);

    InputEvent event;
    for(bool processing = true; processing;) {
        // Pops a message off the queue and stores it in `event`.
        // No message priority denoted by NULL, and 100 ticks of timeout.
        FuriStatus status = furi_message_queue_get(app_context->event_queue, &event, 100);
        furi_check(furi_mutex_acquire(app_context->model_mutex, FuriWaitForever) == FuriStatusOk);
        if(status == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyLeft:
                    if(app_context->model->current_symbol->left != NULL) {
                        app_context->model->current_symbol =
                            app_context->model->current_symbol->left;
                        break;
                    }
                    // reset to root
                    app_context->model->current_symbol = app_context->model->tree_root;
                    break;
                case InputKeyRight:
                    if(app_context->model->current_symbol->right != NULL) {
                        app_context->model->current_symbol =
                            app_context->model->current_symbol->right;
                        break;
                    }

                    // reset to root
                    app_context->model->current_symbol = app_context->model->tree_root;

                    break;
                case InputKeyUp:
                case InputKeyDown:
                    // noop for ip and down keys 
                    break;
                case InputKeyOk:
                    // send key to the keyboard
                    // reset the current symbol
                    // todo: add real keyboard codes
                    furi_hal_hid_kb_press(0x14);
                    furi_hal_hid_kb_release(0x14);
                    app_context->model->current_symbol = app_context->model->tree_root;
                    break;
                case InputKeyBack:
                case InputKeyMAX:
                    processing = false;
                    break;
                }
            }
        }
        furi_mutex_release(app_context->model_mutex);
        view_port_update(app_context->view_port); // signals our draw callback
    }

    // Change back profile
    furi_hal_usb_set_config(usb_mode_prev, NULL);

    morse_code_app_free(app_context);
    return 0;
}
