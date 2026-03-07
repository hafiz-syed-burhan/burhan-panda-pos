#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ==================== THEME & BRANDING (FOODPANDA STYLE) ====================
const char *ULTIMATE_CSS = 
    "* { font-family: 'Inter', 'Segoe UI', sans-serif; }"
    "window { background: #F7F7F7; }"
    ".top-header { background: #D70F64; padding: 15px 30px; box-shadow: 0 4px 15px rgba(215, 15, 100, 0.2); }"
    ".logo-text { color: white; font-size: 24px; font-weight: 900; letter-spacing: -1px; }"
    ".search-bar { border-radius: 25px; border: none; padding: 8px 20px; font-size: 13px; }"
    ".sidebar { background: white; border-right: 1px solid #eee; padding: 20px 10px; }"
    ".nav-btn { background: none; color: #333; border-radius: 12px; padding: 12px; border: none; font-weight: bold; text-align: left; margin-bottom: 5px; }"
    ".nav-btn:hover { background: #D70F64; color: #FFFFFF; }"
    ".active-nav { background: #D70F64; color: white; }"
    ".main-content { background: #FFFFFF; border-radius: 30px 30px 0 0; padding: 25px; margin-top: 10px; }"
    ".category-btn { background: #f0f0f0; border-radius: 20px; padding: 8px 18px; border: 1px solid #ddd; font-weight: 600; margin-right: 10px; }"
    ".category-btn:hover { background: #D70F64; color: white; }"
    ".product-card { background: white; border: 1px solid #f0f0f0; border-radius: 20px; padding: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.02); }"
    ".product-card:hover { transform: translateY(-5px); box-shadow: 0 10px 20px rgba(0,0,0,0.05); border-color: #D70F64; }"
    ".price-tag { color: #D70F64; font-size: 16px; font-weight: 800; }"
    ".cart-panel { background: white; padding: 30px; border-left: 1px solid #eee; box-shadow: -10px 0 30px rgba(0,0,0,0.03); }"
    ".total-lbl { font-size: 38px; font-weight: 900; color: #333; }"
    ".btn-checkout { background: #D70F64; color: white; border-radius: 15px; padding: 18px; font-size: 16px; font-weight: 800; border: none; }"
    "treeview { color: #00f2ff !important; background-color: #D3D3D4; font-weight: bold; font-size: 15px; }"
    "treeview header button label { color: #D70F64; font-weight: 800; }";

// ==================== DATA STRUCTURES ====================
typedef struct {
    char *name;
    int price;
    char *category;
} FoodItem;

FoodItem inventory[] = {
    {"Classic Beef Burger", 750, "Burgers"}, {"Zinger Combo", 950, "Burgers"},
    {"Pepperoni Pizza L", 1500, "Pizza"}, {"Margherita M", 1100, "Pizza"},
    {"Alfredo Pasta", 850, "Pasta"}, {"Arrabiata Pasta", 780, "Pasta"},
    {"Iced Caramel Latte", 550, "Drinks"}, {"Coca Cola 500ml", 150, "Drinks"},
    {"Chocolate Lava Cake", 450, "Desserts"}, {"New York Cheesecake", 600, "Desserts"}
};

GtkWidget *main_stack, *cart_view, *total_label, *search_entry;
GtkListStore *cart_store;
int total_bill = 0;

// ==================== APP LOGIC ====================

void update_total_display() {
    char buf[64];
    sprintf(buf, "Rs. %d", total_bill);
    gtk_label_set_text(GTK_LABEL(total_label), buf);
}

void on_add_to_cart(GtkWidget *w, gpointer data) {
    int idx = GPOINTER_TO_INT(data);
    GtkTreeIter iter;
    gtk_list_store_append(cart_store, &iter);
    gtk_list_store_set(cart_store, &iter, 0, inventory[idx].name, 1, inventory[idx].price, -1);
    total_bill += inventory[idx].price;
    update_total_display();
}

void on_nav_click(GtkWidget *b, gpointer page_id) {
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), (const char *)page_id);
}

void on_checkout(GtkWidget *b, gpointer d) {
    if (total_bill == 0) return;
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Order Confirmed! 🐼");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Burhan Panda is preparing your food.\nTotal Amount: Rs. %d", total_bill);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    gtk_list_store_clear(cart_store);
    total_bill = 0;
    update_total_display();
}

// ==================== PAGE CREATORS ====================

GtkWidget* create_dashboard() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    GtkWidget *cat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    const char *cats[] = {"All Items", "Burgers", "Pizza", "Pasta", "Drinks"};
    for(int i=0; i<5; i++) {
        GtkWidget *btn = gtk_button_new_with_label(cats[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "category-btn");
        gtk_box_pack_start(GTK_BOX(cat_box), btn, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(vbox), cat_box, FALSE, FALSE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    for(int i=0; i<10; i++) {
        GtkWidget *card = gtk_button_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "product-card");
        gtk_widget_set_size_request(card, 200, 150);
        
        GtkWidget *cv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        GtkWidget *name = gtk_label_new(inventory[i].name);
        gtk_label_set_xalign(GTK_LABEL(name), 0);
        char p[32]; sprintf(p, "Rs. %d", inventory[i].price);
        GtkWidget *price = gtk_label_new(p);
        gtk_style_context_add_class(gtk_widget_get_style_context(price), "price-tag");
        gtk_label_set_xalign(GTK_LABEL(price), 0);
        
        gtk_box_pack_start(GTK_BOX(cv), name, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(cv), price, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(card), cv);
        
        g_signal_connect(card, "clicked", G_CALLBACK(on_add_to_cart), GINT_TO_POINTER(i));
        gtk_grid_attach(GTK_GRID(grid), card, i%3, i/3, 1, 1);
    }
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);
    return scroll;
}

GtkWidget* create_orders_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 30);
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='#D70F64'>Order History</span>");
    
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "#BK9921", 1, "March 07, 2026", 2, "Delivered", -1);
    
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Order ID", r, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Date", r, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Status", r, "text", 2, NULL);

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), tree, TRUE, TRUE, 0);
    return box;
}

GtkWidget* create_profile_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(box), 40);
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='#D70F64'>Profile Settings</span>");
    
    GtkWidget *name = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Full Name");
    GtkWidget *email = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(email), "Email Address");
    GtkWidget *save = gtk_button_new_with_label("Update Profile");
    gtk_style_context_add_class(gtk_widget_get_style_context(save), "btn-checkout");

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), name, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), email, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), save, FALSE, FALSE, 15);
    return box;
}

GtkWidget* create_settings_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 40);
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='#D70F64'>App Configurations</span>");

    GtkWidget *sw_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *sw_lbl = gtk_label_new("Enable Dark Mode (VIP)");
    GtkWidget *sw = gtk_switch_new();
    gtk_box_pack_start(GTK_BOX(sw_box), sw_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sw_box), sw, FALSE, FALSE, 0);

    GtkWidget *lang_lbl = gtk_label_new("Language Selection:");
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "English");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Urdu (Roman)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), sw_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), lang_lbl, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 5);
    return box;
}

// ==================== MAIN UI ASSEMBLY ====================

void build_main_ui() {
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), 1100, 750);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), root);

    // --- Header ---
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "top-header");
    GtkWidget *logo = gtk_label_new("Burhan Panda 🐼");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "logo-text");
    gtk_box_pack_start(GTK_BOX(header), logo, FALSE, FALSE, 0);
    
    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search for restaurants...");
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "search-bar");
    gtk_widget_set_size_request(search_entry, 450, -1);
    gtk_box_pack_start(GTK_BOX(header), search_entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(root), main_hbox, TRUE, TRUE, 0);

    // --- Sidebar ---
    GtkWidget *side = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(side), "sidebar");
    gtk_widget_set_size_request(side, 200, -1);
    
    const char *nav_items[] = {"🏠 Dashboard", "📜 My Orders", "👤 Profile", "⚙ Settings"};
    const char *nav_ids[] = {"dash", "orders", "prof", "set"};
    for(int i=0; i<4; i++) {
        GtkWidget *b = gtk_button_new_with_label(nav_items[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(b), "nav-btn");
        g_signal_connect(b, "clicked", G_CALLBACK(on_nav_click), (gpointer)nav_ids[i]);
        gtk_box_pack_start(GTK_BOX(side), b, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(main_hbox), side, FALSE, FALSE, 0);

    // --- Center Stack (FIXED) ---
    main_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(main_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    
    gtk_stack_add_named(GTK_STACK(main_stack), create_dashboard(), "dash");
    gtk_stack_add_named(GTK_STACK(main_stack), create_orders_page(), "orders");
    gtk_stack_add_named(GTK_STACK(main_stack), create_profile_page(), "prof");
    gtk_stack_add_named(GTK_STACK(main_stack), create_settings_page(), "set");
    
    gtk_box_pack_start(GTK_BOX(main_hbox), main_stack, TRUE, TRUE, 0);

    // --- Right Cart Panel ---
    GtkWidget *cart = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_style_context_add_class(gtk_widget_get_style_context(cart), "cart-panel");
    gtk_widget_set_size_request(cart, 350, -1);
    
    GtkWidget *basket_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(basket_label), "<b>Your Basket</b>");
    gtk_box_pack_start(GTK_BOX(cart), basket_label, FALSE, FALSE, 0);
    
    cart_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    cart_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cart_store));
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cart_view), -1, "Item", r, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cart_view), -1, "Price", r, "text", 1, NULL);
    
    GtkWidget *cs = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(cs), cart_view);
    gtk_box_pack_start(GTK_BOX(cart), cs, TRUE, TRUE, 0);

    total_label = gtk_label_new("Rs. 0");
    gtk_style_context_add_class(gtk_widget_get_style_context(total_label), "total-lbl");
    gtk_box_pack_start(GTK_BOX(cart), total_label, FALSE, FALSE, 10);

    GtkWidget *checkout_btn = gtk_button_new_with_label("Checkout");
    gtk_style_context_add_class(gtk_widget_get_style_context(checkout_btn), "btn-checkout");
    g_signal_connect(checkout_btn, "clicked", G_CALLBACK(on_checkout), NULL);
    gtk_box_pack_start(GTK_BOX(cart), checkout_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_hbox), cart, FALSE, FALSE, 0);
    gtk_widget_show_all(win);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, ULTIMATE_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cp), 800);
    build_main_ui();
    gtk_main();
    return 0;
}
