#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h> // SQLite Library

// ==================== DATABASE SECTION ====================
sqlite3 *db;

void init_database() {
    int rc = sqlite3_open("burhan_panda.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return;
    }
    char *err_msg = 0;
    // Orders table create karna agar exist nahi karti
    const char *sql = "CREATE TABLE IF NOT EXISTS orders("
                      "id TEXT, date TEXT, status TEXT, total INT);";
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (err_msg) {
        printf("SQL Error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

void save_order_to_db(const char* id, int total) {
    char sql[256];
    // Database mein order insert karne ki query
    sprintf(sql, "INSERT INTO orders VALUES('%s', datetime('now', 'localtime'), 'Preparing', %d);", id, total);
    sqlite3_exec(db, sql, 0, 0, 0);
}

// ==================== THEME & BRANDING ====================
const char *ULTIMATE_CSS = 
    //"* { font-family: 'Inter', 'Segoe UI', sans-serif; transition: all 0.3s ease; }"
    "* { font-family: 'Open Sans', 'Segoe UI', sans-serif; }"
    "window { background: #F7F7F7; }"
    //".top-header { background: #D70F64; padding: 15px 30px; box-shadow: 0 4px 15px rgba(215, 15, 100, 0.2); }"
    //.top-header { background: #E60050; padding: 15px 30px; box-shadow: 0 4px 15px rgba(230, 0, 80, 0.3); }"
    //".top-header { background: #e21b70; padding: 10px 25px; border-bottom: 1px solid rgba(0,0,0,0.05); box-shadow: 0 2px 4px rgba(0,0,0,0.08); }"
    ".top-header { background: #e21b70; padding: 12px 40px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); }"
    //".logo-text { color: white; font-weight: 800; font-size: 22px; }"
    ".logo-text { color: white; font-size: 24px; font-weight: 900; letter-spacing: -1px; }"
    ".search-bar { border-radius: 25px; border: none; padding: 8px 20px; font-size: 13px; }"
    ".sidebar { background: white; border-right: 1px solid #eee; padding: 20px 10px; }"
    ".nav-btn { background: none; color: #333; border-radius: 12px; padding: 12px; border: none; font-weight: 800; font-size: 16px; text-align: left; margin-bottom: 5px; }"
    ".nav-btn:hover { background: #e21b70; color: #FFFFFF; }"
    ".main-content { background: #FFFFFF; border-radius: 30px 30px 0 0; padding: 25px; margin-top: 10px; }"
    ".category-btn { background: #f0f0f0; border-radius: 20px; padding: 8px 18px; border: 1px solid #ddd; font-weight: 600; margin-right: 10px; }"
    ".category-btn:hover { background: #e21b70; color: white; }"
    ".product-card { background: white; border: 1px solid #f0f0f0; border-radius: 20px; padding: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.02); }"
    ".product-card:hover { border: 2px solid #e21b70; background: #FFF5F8; box-shadow: 0 6px 15px rgba(215, 15, 100, 0.1); }"
    ".product-card:active { background: #e0e0e0; }"
    ".price-tag { color: #e21b70; font-size: 16px; font-weight: 800; }"
    ".cart-panel { background: white; padding: 30px; border-left: 1px solid #eee; box-shadow: -10px 0 30px rgba(0,0,0,0.03); }"
    ".total-lbl { font-size: 38px; font-weight: 900; color: #333; }"
    ".btn-checkout { background: #e21b70; color: white; border-radius: 15px; padding: 18px; font-size: 16px; font-weight: 800; border: none; }"
    ".dark-mode { background: #1a1a1a; color: white; }"
    ".dark-mode window { background: #121212; }"
    ".dark-mode .sidebar, .dark-mode .cart-panel { background: #1e1e1e; border-color: #333; }"
    ".dark-mode .nav-btn, .dark-mode .total-lbl { color: #eee; }"
    "treeview { color: #00f2ff !important; background-color: #D3D3D4; font-weight: bold; font-size: 15px; }"
    "treeview header button label { color: #e21b70; font-weight: 800; }";

// ==================== DATA STRUCTURES ====================
// Line 73: Struct mein img_path add karen
typedef struct {
    char *name;
    int price;
    char *category;
    char *img_path; // <--- Naya field add kiya
} FoodItem;

// Line 80: Inventory array mein paths add karen
FoodItem inventory[] = {
    {"Classic Beef Burger", 750, "Burgers", "images/burger.png"},
    {"Zinger Combo", 950, "Burgers", "images/zinger.png"},
    {"Double Cheese Patty", 850, "Burgers", "images/double.png"},
    {"Pepperoni Pizza L", 1500, "Pizza", "images/pizza.png"},
    {"Margherita M", 1100, "Pizza", "images/margherita.png"},
    {"Fajita Passion", 1300, "Pizza", "images/fajita.png"},
    {"Alfredo Pasta", 850, "Pasta", "images/pasta.png"},
    {"Arrabiata Pasta", 780, "Pasta", "images/arrabiata.png"},
    {"Mac n Cheese", 700, "Pasta", "images/mac.png"},
    {"Chicken Tikka", 450, "BBQ", "images/tikka.png"},
    {"Beef Seekh Kabab", 550, "BBQ", "images/kabab.png"},
    {"Malai Boti", 600, "BBQ", "images/malai.png"},
    {"Beef Biryani", 650, "Rice", "images/biryani.png"},
    {"Chicken Pulao", 500, "Rice", "images/pulao.png"},
    {"Iced Caramel Latte", 550, "Drinks", "images/coffee.png"},
    {"Coca Cola 500ml", 150, "Drinks", "images/coke.png"},
    {"Fresh Lime", 200, "Drinks", "images/lime.png"},
    {"Chocolate Lava Cake", 450, "Desserts", "images/cake.png"},
    {"New York Cheesecake", 600, "Desserts", "images/cheesecake.png"},
    {"Gulab Jamun (2pc)", 250, "Desserts", "images/jamun.png"}
};

GtkWidget *main_stack, *cart_view, *total_label, *search_entry, *menu_grid, *order_history_tree, *profile_pixbuf_widget;
GtkListStore *cart_store, *order_history_store;
int total_bill = 0;
char *current_category = "All Items";



// ==================== IMAGE HELPER SECTION ====================
GtkWidget* create_product_image(const char *path) {
    // Image ko resize karne ke liye Pixbuf use kiya hai
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 150, 110, TRUE, NULL);
    GtkWidget *img;
    
    if (pb) {
        img = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb); // Memory clean karne ke liye
    } else {
        
        img = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
    }
    return img;
}



// ==================== APP LOGIC ====================

void update_total_display() {
    char buf[64];
    sprintf(buf, "Rs. %d", total_bill);
    gtk_label_set_text(GTK_LABEL(total_label), buf);
}



// ==================== CART OPERATIONS ====================
void on_remove_item(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        int price;
        gtk_tree_model_get(model, &iter, 1, &price, -1);
        
        total_bill -= price;
        update_total_display(); // Ab compiler ko pata hai ke ye function upar maujood hai

        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}


void on_add_to_cart(GtkWidget *w, gpointer data) {
    int idx = GPOINTER_TO_INT(data);
    GtkTreeIter iter;
    gtk_list_store_append(cart_store, &iter);
    gtk_list_store_set(cart_store, &iter, 0, inventory[idx].name, 1, inventory[idx].price, -1);
    total_bill += inventory[idx].price;
    update_total_display();
}

void refresh_menu(const char *search_query) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu_grid));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    int count = 0;
    for (int i = 0; i < 20; i++) {
        int cat_match = (strcmp(current_category, "All Items") == 0 || strcmp(inventory[i].category, current_category) == 0);
        int search_match = (search_query == NULL || strlen(search_query) == 0 || 
                           g_strrstr(g_ascii_strdown(inventory[i].name, -1), g_ascii_strdown(search_query, -1)));

        if (cat_match && search_match) {
            GtkWidget *card = gtk_button_new();
            gtk_style_context_add_class(gtk_widget_get_style_context(card), "product-card");
            gtk_widget_set_size_request(card, 200, 320);
            GtkWidget *cv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12); // Spacing 8 rakhen behtar look ke liye

// 1. Image create karen (Helper function call ho raha hai)
            GtkWidget *img = create_product_image(inventory[i].img_path);

// 2. Name aur Price labels banayen
            GtkWidget *name = gtk_label_new(inventory[i].name);
            char p[32]; sprintf(p, "Rs. %d", inventory[i].price);
            GtkWidget *price = gtk_label_new(p);
            gtk_style_context_add_class(gtk_widget_get_style_context(price), "price-tag");

// 3. PACKING ORDER (Boht zaroori hai):
            gtk_box_pack_start(GTK_BOX(cv), img, TRUE, TRUE, 0);   // Sabse upar Image
            gtk_box_pack_start(GTK_BOX(cv), name, FALSE, FALSE, 0); // Beech mein Naam
            gtk_box_pack_start(GTK_BOX(cv), price, FALSE, FALSE, 0); // Sabse niche Price
            gtk_container_add(GTK_CONTAINER(card), cv);
            g_signal_connect(card, "clicked", G_CALLBACK(on_add_to_cart), GINT_TO_POINTER(i));
            gtk_grid_attach(GTK_GRID(menu_grid), card, count % 5, count / 5, 1, 1);
            count++;
        }
    }
    gtk_widget_show_all(menu_grid);
}

void on_category_click(GtkWidget *btn, gpointer data) {
    current_category = (char *)data;
    const char *search_text = gtk_entry_get_text(GTK_ENTRY(search_entry));
    refresh_menu(search_text);
}

void on_search_changed(GtkEditable *editable, gpointer user_data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    refresh_menu(text);
}

void on_nav_click(GtkWidget *b, gpointer page_id) {
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), (const char *)page_id);
}

void on_checkout(GtkWidget *b, gpointer d) {
    if (total_bill == 0) return;
    GtkTreeIter iter;
    char order_id[16];
    sprintf(order_id, "#BK%d", 1000 + rand()%9000);
    
    // SQLite mein save karna
    save_order_to_db(order_id, total_bill);

    gtk_list_store_append(order_history_store, &iter);
    gtk_list_store_set(order_history_store, &iter, 0, order_id, 1, "Just Now", 2, "Preparing", -1);

    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Order Confirmed! 🐼");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Burhan Panda is preparing your food.\nOrder ID: %s | Total: Rs. %d\n(Saved to Database)", order_id, total_bill);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    gtk_list_store_clear(cart_store);
    total_bill = 0;
    update_total_display();
}

void on_image_upload_clicked(GtkWidget *btn, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Profile Picture", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(filename, 120, 120, TRUE, NULL);
        if (pb) gtk_image_set_from_pixbuf(GTK_IMAGE(profile_pixbuf_widget), pb);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_profile_update(GtkWidget *btn, gpointer data) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Profile Updated!");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Your settings have been saved.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_dark_mode_toggled(GtkSwitch *sw, gboolean state, gpointer user_data) {
    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(user_data));
    if (state) gtk_style_context_add_class(context, "dark-mode");
    else gtk_style_context_remove_class(context, "dark-mode");
}

// ==================== PAGE CREATORS ====================

GtkWidget* create_dashboard() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    GtkWidget *cat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    const char *cats[] = {"All Items", "Burgers", "Pizza", "Pasta", "BBQ", "Rice", "Drinks", "Desserts"};
    for(int i=0; i<8; i++) {
        GtkWidget *btn = gtk_button_new_with_label(cats[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "category-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_category_click), (gpointer)cats[i]);
        gtk_box_pack_start(GTK_BOX(cat_box), btn, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(vbox), cat_box, FALSE, FALSE, 0);

    menu_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(menu_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(menu_grid), 20);
    refresh_menu(""); 
    gtk_box_pack_start(GTK_BOX(vbox), menu_grid, TRUE, TRUE, 0);
    return scroll;
}

GtkWidget* create_orders_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 30);
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='#D70F64'>Order History</span>");
    
    order_history_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(order_history_store));
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

    profile_pixbuf_widget = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);
    GtkWidget *img_btn = gtk_button_new_with_label("📷 Upload Photo");
    gtk_style_context_add_class(gtk_widget_get_style_context(img_btn), "category-btn");
    g_signal_connect(img_btn, "clicked", G_CALLBACK(on_image_upload_clicked), NULL);

    GtkWidget *name = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Full Name");
    GtkWidget *email = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(email), "Email Address");
    GtkWidget *save = gtk_button_new_with_label("Update Profile");
    gtk_style_context_add_class(gtk_widget_get_style_context(save), "btn-checkout");
    g_signal_connect(save, "clicked", G_CALLBACK(on_profile_update), NULL);

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), profile_pixbuf_widget, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), img_btn, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), name, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), email, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), save, FALSE, FALSE, 15);
    return box;
}

GtkWidget* create_settings_page(GtkWidget *win) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 40);
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='#D70F64'>App Configurations</span>");
    GtkWidget *sw_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *sw_lbl = gtk_label_new("Enable Dark Mode (VIP)");
    GtkWidget *sw = gtk_switch_new();
    g_signal_connect(sw, "state-set", G_CALLBACK(on_dark_mode_toggled), win);

    gtk_box_pack_start(GTK_BOX(sw_box), sw_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sw_box), sw, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), sw_box, FALSE, FALSE, 5);
    return box;
}

// ==================== MAIN UI ASSEMBLY ====================

void build_main_ui() {
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Burhan Panda - Food Management");
    gtk_window_set_default_size(GTK_WINDOW(win), 1100, 750);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), root);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "top-header");
    GtkWidget *logo = gtk_label_new("Burhan Panda 🐼");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "logo-text");
    gtk_box_pack_start(GTK_BOX(header), logo, FALSE, FALSE, 0);
    
    // 2. SPACER YAHAN LAGAYEIN (Beech mein)
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(header), spacer, TRUE, TRUE, 0);
 
    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search for foods...");
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(search_entry), GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "search-bar");
    gtk_widget_set_size_request(search_entry, 450, -1);
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);

    gtk_box_pack_start(GTK_BOX(header), search_entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(root), main_hbox, TRUE, TRUE, 0);

    GtkWidget *side = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(side), "sidebar");
    const char *nav_items[] = {"🏠 Dashboard", "📜 My Orders", "👤 Profile", "⚙ Settings"};
    const char *nav_ids[] = {"dash", "orders", "prof", "set"};
    for(int i=0; i<4; i++) {
        GtkWidget *b = gtk_button_new_with_label(nav_items[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(b), "nav-btn");
        g_signal_connect(b, "clicked", G_CALLBACK(on_nav_click), (gpointer)nav_ids[i]);
        gtk_box_pack_start(GTK_BOX(side), b, FALSE, FALSE, 0);
    }

//--------------------------------------------------------------    
    //--------------------------------------------------------------    
    // Partition Line
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(side), sep, FALSE, FALSE, 10);

    // 2. Panda Image (Path asan karke 'panda.png' use karein)
    GdkPixbuf *p_pb = gdk_pixbuf_new_from_file("/home/syed-burhan/Downloads/panda.png", NULL);

    if (p_pb) {
        // Size thora mazeed chota rakhein taake fit aa jaye
        GdkPixbuf *p_scaled = gdk_pixbuf_scale_simple(p_pb, 100, 100, GDK_INTERP_BILINEAR);
        GtkWidget *p_img = gtk_image_new_from_pixbuf(p_scaled);
        
        // 'pack_start' use karke dekhte hain agar 'pack_end' se gayab ho rahi hai
        gtk_box_pack_start(GTK_BOX(side), p_img, FALSE, FALSE, 10); 
        
        g_object_unref(p_pb);
        g_object_unref(p_scaled);
        gtk_widget_show(p_img);
    }

    // Sidebar ko main layout mein daalna
    //gtk_box_pack_start(GTK_BOX(main_hbox), side, FALSE, FALSE, 0);

//--------------------------------------------------------------
    //--------------------------------------------
    gtk_box_pack_start(GTK_BOX(main_hbox), side, FALSE, FALSE, 0);

    main_stack = gtk_stack_new();
    gtk_stack_add_named(GTK_STACK(main_stack), create_dashboard(), "dash");
    gtk_stack_add_named(GTK_STACK(main_stack), create_orders_page(), "orders");
    gtk_stack_add_named(GTK_STACK(main_stack), create_profile_page(), "prof");
    gtk_stack_add_named(GTK_STACK(main_stack), create_settings_page(win), "set");
    gtk_box_pack_start(GTK_BOX(main_hbox), main_stack, TRUE, TRUE, 0);

    GtkWidget *cart = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_style_context_add_class(gtk_widget_get_style_context(cart), "cart-panel");
    gtk_widget_set_size_request(cart, 350, -1);
    
    GtkWidget *basket_label = gtk_label_new(NULL);
    //gtk_label_set_markup(GTK_LABEL(basket_label), "<b>Your Basket</b>");
    //gtk_label_set_markup(GTK_LABEL(basket_label), "<span size='x-large' weight='heavy' color='#333'>Your Basket</span>");
    gtk_label_set_markup(GTK_LABEL(basket_label), "<span size='x-large' weight='heavy'>Your Basket <span size='xx-large'>🛒</span></span>");
    gtk_box_pack_start(GTK_BOX(cart), basket_label, FALSE, FALSE, 0);

    cart_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    cart_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cart_store));
    g_signal_connect(cart_view, "row-activated", G_CALLBACK(on_remove_item), NULL);
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
    
    // Database initialize karna
    init_database();
    
    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, ULTIMATE_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cp), 800);
    
    build_main_ui();
    gtk_main();
    
    // Database band karna jab app close ho
    sqlite3_close(db);
    return 0;
}
