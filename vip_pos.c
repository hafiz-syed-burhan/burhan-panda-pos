// ============================================================
//   BURHAN PANDA FOOD MANAGEMENT SYSTEM  v2.0 - UPGRADED
//   New Features: Login, Quantity Control, Coupon System,
//   Payment Selection, Receipt Generator, Feedback System,
//   Analytics Page, Favorites, Notifications, Order Tracker
// ============================================================
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

// ==================== DATABASE SECTION ====================
sqlite3 *db;

void init_database() {
    int rc = sqlite3_open("burhan_panda.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return;
    }
    char *err_msg = 0;

    // Orders table
    const char *sql1 = "CREATE TABLE IF NOT EXISTS orders("
                       "id TEXT, date TEXT, status TEXT, total INT, "
                       "payment_method TEXT, order_type TEXT, feedback INT);";
    sqlite3_exec(db, sql1, 0, 0, &err_msg);

    // Favorites table
    const char *sql2 = "CREATE TABLE IF NOT EXISTS favorites("
                       "item_name TEXT UNIQUE);";
    sqlite3_exec(db, sql2, 0, 0, &err_msg);

    // Users table (simple login system)
    const char *sql3 = "CREATE TABLE IF NOT EXISTS users("
                       "username TEXT UNIQUE, password TEXT, fullname TEXT, email TEXT);";
    sqlite3_exec(db, sql3, 0, 0, &err_msg);

    // Insert default admin user if not exists
    const char *sql4 = "INSERT OR IGNORE INTO users VALUES('admin','1234','Burhan Ahmed','admin@panda.pk');";
    sqlite3_exec(db, sql4, 0, 0, &err_msg);

    // Coupons table
    const char *sql5 = "CREATE TABLE IF NOT EXISTS coupons("
                       "code TEXT UNIQUE, discount_percent INT, min_order INT, used INT DEFAULT 0);";
    sqlite3_exec(db, sql5, 0, 0, &err_msg);

    // Insert some default coupons
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('PANDA10',10,500,0);", 0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('SAVE20',20,1000,0);", 0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('FLAT50',50,2000,0);", 0, 0, 0);

    if (err_msg) { sqlite3_free(err_msg); }
}

void save_order_to_db(const char *id, int total, const char *payment, const char *order_type) {
    char sql[512];
    sprintf(sql, "INSERT INTO orders VALUES('%s', datetime('now','localtime'), 'Preparing', %d, '%s', '%s', 0);",
            id, total, payment, order_type);
    sqlite3_exec(db, sql, 0, 0, 0);
}

int verify_login(const char *user, const char *pass) {
    char sql[256];
    sprintf(sql, "SELECT count(*) FROM users WHERE username='%s' AND password='%s';", user, pass);
    sqlite3_stmt *stmt;
    int found = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            found = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return found;
}

int check_coupon(const char *code, int bill_total) {
    char sql[256];
    sprintf(sql, "SELECT discount_percent, min_order FROM coupons WHERE code='%s' AND used=0;", code);
    sqlite3_stmt *stmt;
    int discount = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int pct = sqlite3_column_int(stmt, 0);
            int min = sqlite3_column_int(stmt, 1);
            if (bill_total >= min) discount = pct;
        }
        sqlite3_finalize(stmt);
    }
    return discount;
}

void mark_coupon_used(const char *code) {
    char sql[128];
    sprintf(sql, "UPDATE coupons SET used=1 WHERE code='%s';", code);
    sqlite3_exec(db, sql, 0, 0, 0);
}

int get_total_orders() {
    sqlite3_stmt *stmt;
    int count = 0;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM orders;", -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return count;
}

int get_total_revenue() {
    sqlite3_stmt *stmt;
    int rev = 0;
    if (sqlite3_prepare_v2(db, "SELECT IFNULL(SUM(total),0) FROM orders;", -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) rev = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return rev;
}

void toggle_favorite(const char *item_name) {
    char check_sql[256], del_sql[256], ins_sql[256];
    sprintf(check_sql, "SELECT count(*) FROM favorites WHERE item_name='%s';", item_name);
    sqlite3_stmt *stmt;
    int exists = 0;
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (exists) {
        sprintf(del_sql, "DELETE FROM favorites WHERE item_name='%s';", item_name);
        sqlite3_exec(db, del_sql, 0, 0, 0);
    } else {
        sprintf(ins_sql, "INSERT OR IGNORE INTO favorites VALUES('%s');", item_name);
        sqlite3_exec(db, ins_sql, 0, 0, 0);
    }
}

int is_favorite(const char *item_name) {
    char sql[256];
    sprintf(sql, "SELECT count(*) FROM favorites WHERE item_name='%s';", item_name);
    sqlite3_stmt *stmt;
    int res = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) res = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return res;
}

// ==================== THEME & BRANDING ====================
const char *ULTIMATE_CSS =
    "* { font-family: 'Open Sans', 'Segoe UI', sans-serif; }"
    "window { background: #F7F7F7; }"
    ".top-header { background: #e21b70; padding: 12px 40px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); }"
    ".logo-text { color: white; font-size: 24px; font-weight: 900; letter-spacing: -1px; }"
    ".search-bar { border-radius: 25px; border: none; padding: 8px 20px; font-size: 13px; }"
    ".sidebar { background: white; border-right: 1px solid #eee; padding: 20px 10px; min-width: 170px; }"
    ".nav-btn { background: none; color: #333; border-radius: 12px; padding: 12px; border: none; font-weight: 800; font-size: 15px; text-align: left; margin-bottom: 5px; }"
    ".nav-btn:hover { background: #e21b70; color: #FFFFFF; }"
    ".main-content { background: #FFFFFF; border-radius: 30px 30px 0 0; padding: 25px; margin-top: 10px; }"
    ".category-btn { background: #f0f0f0; border-radius: 20px; padding: 8px 18px; border: 1px solid #ddd; font-weight: 600; margin-right: 10px; }"
    ".category-btn:hover { background: #e21b70; color: white; }"
    ".product-card { background: white; border: 1px solid #f0f0f0; border-radius: 20px; padding: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.02); }"
    ".product-card:hover { border: 2px solid #e21b70; background: #FFF5F8; box-shadow: 0 6px 15px rgba(215,15,100,0.1); }"
    ".price-tag { color: #e21b70; font-size: 16px; font-weight: 800; }"
    ".cart-panel { background: white; padding: 20px; border-left: 1px solid #eee; box-shadow: -10px 0 30px rgba(0,0,0,0.03); }"
    ".total-lbl { font-size: 30px; font-weight: 900; color: #333; }"
    ".discount-lbl { font-size: 14px; font-weight: 700; color: #27ae60; }"
    ".btn-checkout { background: #e21b70; color: white; border-radius: 15px; padding: 18px; font-size: 16px; font-weight: 800; border: none; }"
    ".btn-danger { background: #e74c3c; color: white; border-radius: 10px; padding: 8px 15px; font-size: 13px; font-weight: 700; border: none; }"
    ".btn-success { background: #27ae60; color: white; border-radius: 10px; padding: 8px 15px; font-size: 13px; font-weight: 700; border: none; }"
    ".btn-fav { background: none; border: 1px solid #e21b70; color: #e21b70; border-radius: 20px; padding: 3px 10px; font-size: 11px; font-weight: 700; }"
    ".qty-btn { background: #e21b70; color: white; border-radius: 8px; padding: 2px 10px; font-size: 16px; font-weight: 900; border: none; min-width: 30px; }"
    ".qty-lbl { font-size: 14px; font-weight: 800; color: #333; margin: 0 8px; }"
    ".stat-card { background: white; border-radius: 20px; padding: 20px; border: 1px solid #f0f0f0; box-shadow: 0 4px 10px rgba(0,0,0,0.04); }"
    ".stat-number { font-size: 32px; font-weight: 900; color: #e21b70; }"
    ".stat-label { font-size: 13px; color: #888; font-weight: 600; }"
    ".notification-bar { background: #27ae60; padding: 10px 20px; border-radius: 10px; margin: 5px; }"
    ".notif-text { color: white; font-weight: 700; font-size: 13px; }"
    ".order-type-btn { background: #f5f5f5; border-radius: 12px; padding: 10px 20px; border: 2px solid #ddd; font-weight: 700; font-size: 14px; }"
    ".order-type-btn-active { background: #e21b70; color: white; border-radius: 12px; padding: 10px 20px; border: 2px solid #e21b70; font-weight: 700; font-size: 14px; }"
    ".login-card { background: white; border-radius: 25px; padding: 40px; box-shadow: 0 15px 50px rgba(0,0,0,0.1); }"
    ".login-entry { border-radius: 12px; padding: 10px; font-size: 15px; border: 2px solid #eee; }"
    ".login-btn { background: #e21b70; color: white; border-radius: 15px; padding: 15px; font-size: 16px; font-weight: 800; border: none; }"
    ".progress-step-done { background: #27ae60; color: white; border-radius: 20px; padding: 5px 14px; font-size: 12px; font-weight: 700; }"
    ".progress-step-active { background: #e21b70; color: white; border-radius: 20px; padding: 5px 14px; font-size: 12px; font-weight: 700; }"
    ".progress-step-todo { background: #eee; color: #888; border-radius: 20px; padding: 5px 14px; font-size: 12px; font-weight: 700; }"
    "treeview { color: #333; background-color: #fafafa; font-size: 14px; }"
    "treeview header button label { color: #e21b70; font-weight: 800; }";

// ==================== DATA STRUCTURES ====================
typedef struct {
    char *name;
    int price;
    char *category;
    char *img_path;
} FoodItem;

typedef struct {
    char name[128];
    int price;
    int quantity;
} CartItem;

FoodItem inventory[] = {
    {"Classic Beef Burger",   750,  "Burgers",  "images/burger.png"},
    {"Zinger Combo",          950,  "Burgers",  "images/zinger.png"},
    {"Double Cheese Patty",   850,  "Burgers",  "images/double.png"},
    {"Pepperoni Pizza L",     1500, "Pizza",    "images/pizza.png"},
    {"Margherita M",          1100, "Pizza",    "images/margherita.png"},
    {"Fajita Passion",        1300, "Pizza",    "images/fajita.png"},
    {"Alfredo Pasta",         850,  "Pasta",    "images/pasta.png"},
    {"Arrabiata Pasta",       780,  "Pasta",    "images/arrabiata.png"},
    {"Mac n Cheese",          700,  "Pasta",    "images/mac.png"},
    {"Chicken Tikka",         450,  "BBQ",      "images/tikka.png"},
    {"Beef Seekh Kabab",      550,  "BBQ",      "images/kabab.png"},
    {"Malai Boti",            600,  "BBQ",      "images/malai.png"},
    {"Beef Biryani",          650,  "Rice",     "images/biryani.png"},
    {"Chicken Pulao",         500,  "Rice",     "images/pulao.png"},
    {"Iced Caramel Latte",    550,  "Drinks",   "images/coffee.png"},
    {"Coca Cola 500ml",       150,  "Drinks",   "images/coke.png"},
    {"Fresh Lime",            200,  "Drinks",   "images/lime.png"},
    {"Chocolate Lava Cake",   450,  "Desserts", "images/cake.png"},
    {"New York Cheesecake",   600,  "Desserts", "images/cheesecake.png"},
    {"Gulab Jamun (2pc)",     250,  "Desserts", "images/jamun.png"}
};

// ==================== GLOBAL WIDGETS & STATE ====================
GtkWidget *main_window;
GtkWidget *main_stack, *cart_view, *total_label, *search_entry;
GtkWidget *menu_grid, *order_history_tree, *profile_pixbuf_widget;
GtkWidget *discount_label, *coupon_entry, *notif_revealer, *notif_label;
GtkWidget *order_type_dine_btn, *order_type_take_btn;
GtkWidget *analytics_orders_lbl, *analytics_revenue_lbl, *analytics_avg_lbl;

GtkListStore *cart_store, *order_history_store;

int total_bill       = 0;
int discount_percent = 0;
int discount_amount  = 0;
char *current_category = "All Items";
char current_order_type[32] = "Dine-In";  // or Takeaway
char current_payment[32]    = "Cash";
char logged_in_user[64]     = "";
int  notif_timeout_id       = 0;

// Cart items array for quantity management
CartItem cart_items[100];
int cart_item_count = 0;

// ==================== FORWARD DECLARATIONS ====================
void update_total_display();
void refresh_menu(const char *search_query);
void refresh_cart_view();

// ==================== NOTIFICATION SYSTEM ====================
gboolean hide_notification(gpointer data) {
    gtk_revealer_set_reveal_child(GTK_REVEALER(notif_revealer), FALSE);
    notif_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void show_notification(const char *message, const char *color) {
    gtk_label_set_text(GTK_LABEL(notif_label), message);

    // Change notification color dynamically
    char css[256];
    sprintf(css, ".notification-bar { background: %s; padding: 10px 20px; border-radius: 10px; margin: 5px; }", color);
    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, css, -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(notif_revealer),
                                   GTK_STYLE_PROVIDER(cp), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_revealer_set_reveal_child(GTK_REVEALER(notif_revealer), TRUE);
    if (notif_timeout_id) g_source_remove(notif_timeout_id);
    notif_timeout_id = g_timeout_add(3000, hide_notification, NULL);
}

// ==================== IMAGE HELPER ====================
GtkWidget* create_product_image(const char *path) {
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 150, 110, TRUE, NULL);
    GtkWidget *img;
    if (pb) {
        img = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb);
    } else {
        img = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
    }
    return img;
}

// ==================== APP LOGIC ====================
void update_total_display() {
    discount_amount = (total_bill * discount_percent) / 100;
    int final_total = total_bill - discount_amount;
    char buf[64];
    sprintf(buf, "Rs. %d", final_total);
    gtk_label_set_text(GTK_LABEL(total_label), buf);

    if (discount_percent > 0) {
        char disc_buf[64];
        sprintf(disc_buf, "? %d%% OFF  - Rs. %d saved!", discount_percent, discount_amount);
        gtk_label_set_text(GTK_LABEL(discount_label), disc_buf);
        gtk_widget_show(discount_label);
    } else {
        gtk_widget_hide(discount_label);
    }
}

// ==================== CART QUANTITY MANAGEMENT ====================
void on_increase_qty(GtkWidget *btn, gpointer data) {
    int idx = GPOINTER_TO_INT(data);
    if (idx < 0 || idx >= cart_item_count) return;
    cart_items[idx].quantity++;
    total_bill += cart_items[idx].price;
    update_total_display();
    refresh_cart_view();
}

void on_decrease_qty(GtkWidget *btn, gpointer data) {
    int idx = GPOINTER_TO_INT(data);
    if (idx < 0 || idx >= cart_item_count) return;
    if (cart_items[idx].quantity > 1) {
        cart_items[idx].quantity--;
        total_bill -= cart_items[idx].price;
    } else {
        // Remove item completely
        total_bill -= cart_items[idx].price;
        for (int i = idx; i < cart_item_count - 1; i++) {
            cart_items[i] = cart_items[i + 1];
        }
        cart_item_count--;
    }
    update_total_display();
    refresh_cart_view();
    show_notification("Item removed from cart", "#e74c3c");
}

void refresh_cart_view() {
    // Clear existing cart widget children (rebuild cart list)
    // We use GtkListStore approach - clear and re-add
    gtk_list_store_clear(cart_store);
    for (int i = 0; i < cart_item_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(cart_store, &iter);
        char qty_str[16];
        sprintf(qty_str, "x%d", cart_items[i].quantity);
        int line_total = cart_items[i].price * cart_items[i].quantity;
        gtk_list_store_set(cart_store, &iter,
                           0, cart_items[i].name,
                           1, qty_str,
                           2, line_total,
                           -1);
    }
}

void on_add_to_cart(GtkWidget *w, gpointer data) {
    int idx = GPOINTER_TO_INT(data);

    // Check if item already in cart
    int found = -1;
    for (int i = 0; i < cart_item_count; i++) {
        if (strcmp(cart_items[i].name, inventory[idx].name) == 0) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        cart_items[found].quantity++;
    } else {
        if (cart_item_count < 100) {
            strncpy(cart_items[cart_item_count].name, inventory[idx].name, 127);
            cart_items[cart_item_count].price = inventory[idx].price;
            cart_items[cart_item_count].quantity = 1;
            cart_item_count++;
        }
    }
    total_bill += inventory[idx].price;
    update_total_display();
    refresh_cart_view();

    char msg[128];
    sprintf(msg, "? %s added to cart!", inventory[idx].name);
    show_notification(msg, "#27ae60");
}

// ==================== COUPON SYSTEM ====================
void on_apply_coupon(GtkWidget *btn, gpointer data) {
    const char *code = gtk_entry_get_text(GTK_ENTRY(coupon_entry));
    if (strlen(code) == 0) {
        show_notification("Please enter a coupon code!", "#e74c3c");
        return;
    }
    if (total_bill == 0) {
        show_notification("Add items to cart first!", "#e67e22");
        return;
    }
    int disc = check_coupon(code, total_bill);
    if (disc > 0) {
        discount_percent = disc;
        mark_coupon_used(code);
        update_total_display();
        char msg[128];
        sprintf(msg, "?? Coupon applied! %d%% discount!", disc);
        show_notification(msg, "#27ae60");
    } else {
        show_notification("? Invalid or expired coupon!", "#e74c3c");
    }
}

void on_remove_coupon(GtkWidget *btn, gpointer data) {
    discount_percent = 0;
    discount_amount  = 0;
    gtk_entry_set_text(GTK_ENTRY(coupon_entry), "");
    update_total_display();
    show_notification("Coupon removed", "#e67e22");
}

// ==================== ORDER TYPE ====================
void on_select_dine(GtkWidget *btn, gpointer data) {
    strcpy(current_order_type, "Dine-In");
    show_notification("?? Dine-In selected", "#3498db");
}
void on_select_take(GtkWidget *btn, gpointer data) {
    strcpy(current_order_type, "Takeaway");
    show_notification("?? Takeaway selected", "#9b59b6");
}

// ==================== RECEIPT GENERATOR ====================
void generate_receipt(const char *order_id, int subtotal, int discount, int final_total, const char *payment) {
    char filename[64];
    sprintf(filename, "receipt_%s.txt", order_id + 1); // skip '#'

    FILE *f = fopen(filename, "w");
    if (!f) return;

    time_t now = time(NULL);
    char *time_str = ctime(&now);

    fprintf(f, "========================================\n");
    fprintf(f, "        BURHAN PANDA - RECEIPT          \n");
    fprintf(f, "========================================\n");
    fprintf(f, "Order ID   : %s\n", order_id);
    fprintf(f, "Date/Time  : %s", time_str);
    fprintf(f, "Order Type : %s\n", current_order_type);
    fprintf(f, "Payment    : %s\n", payment);
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "%-25s %6s %8s\n", "ITEM", "QTY", "TOTAL");
    fprintf(f, "----------------------------------------\n");
    for (int i = 0; i < cart_item_count; i++) {
        int line = cart_items[i].price * cart_items[i].quantity;
        fprintf(f, "%-25s %6d %8d\n", cart_items[i].name, cart_items[i].quantity, line);
    }
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "%-25s %14d\n", "Subtotal:", subtotal);
    if (discount > 0)
        fprintf(f, "%-25s %14d\n", "Discount:", -discount);
    fprintf(f, "%-25s %14d\n", "TOTAL PAYABLE:", final_total);
    fprintf(f, "========================================\n");
    fprintf(f, "   Thank you for choosing Burhan Panda!\n");
    fprintf(f, "        Come again!   ??\n");
    fprintf(f, "========================================\n");
    fclose(f);
}

// ==================== PAYMENT SELECTION DIALOG ====================
int show_payment_dialog() {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "?? Select Payment Method", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Confirm", GTK_RESPONSE_OK,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);

    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl), "<b><big>Choose Payment Method</big></b>");
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);

    GtkWidget *rb_cash  = gtk_radio_button_new_with_label(NULL,         "?? Cash on Delivery");
    GtkWidget *rb_card  = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "?? Credit / Debit Card");
    GtkWidget *rb_jazz  = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "?? JazzCash");
    GtkWidget *rb_easy  = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "?? EasyPaisa");

    gtk_box_pack_start(GTK_BOX(box), rb_cash,  FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), rb_card,  FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), rb_jazz,  FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), rb_easy,  FALSE, FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_card)))
            strcpy(current_payment, "Card");
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_jazz)))
            strcpy(current_payment, "JazzCash");
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_easy)))
            strcpy(current_payment, "EasyPaisa");
        else
            strcpy(current_payment, "Cash");
    }
    gtk_widget_destroy(dialog);
    return response;
}

// ==================== FEEDBACK DIALOG ====================
void show_feedback_dialog(const char *order_id) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "? Rate Your Experience", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        "_Submit", GTK_RESPONSE_OK,
        "_Skip",   GTK_RESPONSE_CANCEL,
        NULL
    );
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(box), 25);

    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl), "<b><big>How was your experience?</big></b>");
    gtk_label_set_markup(GTK_LABEL(lbl), "<span size='large'>How was your experience?</span>\n<span size='small' color='#888'>Your feedback helps us improve!</span>");

    GtkWidget *stars_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *star_lbl = gtk_label_new("Rating:");

    GtkWidget *spin = gtk_spin_button_new_with_range(1, 5, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 5);

    GtkWidget *star_info = gtk_label_new("(1=Poor  5=Excellent)");

    gtk_box_pack_start(GTK_BOX(stars_box), star_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stars_box), spin, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(stars_box), star_info, FALSE, FALSE, 0);

    GtkWidget *comment_lbl = gtk_label_new("Comments (optional):");
    GtkWidget *comment = gtk_text_view_new();
    gtk_widget_set_size_request(comment, 300, 80);

    gtk_box_pack_start(GTK_BOX(box), lbl,         FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), stars_box,   FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), comment_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), comment,     FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int rating = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
        char sql[256];
        sprintf(sql, "UPDATE orders SET feedback=%d WHERE id='%s';", rating, order_id);
        sqlite3_exec(db, sql, 0, 0, 0);
        show_notification("? Thank you for your feedback!", "#f39c12");
    }
    gtk_widget_destroy(dialog);
}

// ==================== ORDER STATUS TRACKER ====================
void show_order_tracker(const char *order_id) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "?? Order Status Tracker", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_OK,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 300);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 25);

    GtkWidget *title = gtk_label_new(NULL);
    char markup[128];
    sprintf(markup, "<b>Order %s  |  %s</b>", order_id, current_order_type);
    gtk_label_set_markup(GTK_LABEL(title), markup);

    // Progress steps
    GtkWidget *steps_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    const char *steps[] = {"? Received", "? Confirmed", "?? Preparing", "? Ready", "? Delivered"};
    const char *step_classes[] = {"progress-step-done", "progress-step-done", "progress-step-active", "progress-step-todo", "progress-step-todo"};

    for (int i = 0; i < 5; i++) {
        GtkWidget *step_btn = gtk_label_new(steps[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(step_btn), step_classes[i]);
        gtk_box_pack_start(GTK_BOX(steps_box), step_btn, FALSE, FALSE, 0);
        if (i < 4) {
            GtkWidget *arrow = gtk_label_new("?");
            gtk_box_pack_start(GTK_BOX(steps_box), arrow, FALSE, FALSE, 0);
        }
    }

    GtkWidget *eta_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(eta_lbl),
        "<span color='#e21b70' size='large'><b>? Estimated Time: 20-30 mins</b></span>");

    GtkWidget *info = gtk_label_new("Your order is being lovingly prepared by our chefs! ??");

    GtkWidget *progress = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.4);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "40% - Preparing...");

    gtk_box_pack_start(GTK_BOX(box), title,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), steps_box, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), progress,  FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), eta_lbl,   FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), info,       FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ==================== REMOVE ITEM FROM CART (double-click) ====================
void on_remove_item(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    int *indices = gtk_tree_path_get_indices(path);
    int row = indices[0];
    if (row < 0 || row >= cart_item_count) return;

    total_bill -= cart_items[row].price * cart_items[row].quantity;
    for (int i = row; i < cart_item_count - 1; i++) {
        cart_items[i] = cart_items[i + 1];
    }
    cart_item_count--;
    update_total_display();
    refresh_cart_view();
    show_notification("Item removed from cart", "#e74c3c");
}

// ==================== CHECKOUT ====================
void on_checkout(GtkWidget *b, gpointer d) {
    if (total_bill == 0) {
        show_notification("? Cart is empty!", "#e74c3c");
        return;
    }

    // Show payment dialog
    int pay_response = show_payment_dialog();
    if (pay_response == GTK_RESPONSE_CANCEL) return;

    char order_id[16];
    sprintf(order_id, "#BK%d", 1000 + rand() % 9000);

    int subtotal   = total_bill;
    int disc       = (total_bill * discount_percent) / 100;
    int final_total = subtotal - disc;

    // Save to DB
    save_order_to_db(order_id, final_total, current_payment, current_order_type);

    // Add to order history view
    GtkTreeIter iter;
    gtk_list_store_append(order_history_store, &iter);
    gtk_list_store_set(order_history_store, &iter,
                       0, order_id, 1, "Just Now", 2, "Preparing", 3, current_payment, -1);

    // Generate receipt file
    generate_receipt(order_id, subtotal, disc, final_total, current_payment);

    // Confirmation dialog
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "?? Order Confirmed!");
    char secondary[512];
    sprintf(secondary,
        "Order ID   : %s\n"
        "Order Type : %s\n"
        "Payment    : %s\n"
        "Subtotal   : Rs. %d\n"
        "Discount   : Rs. %d\n"
        "Total Paid : Rs. %d\n\n"
        "Receipt saved! Tracking your order... ??",
        order_id, current_order_type, current_payment,
        subtotal, disc, final_total);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // Reset cart
    memset(cart_items, 0, sizeof(cart_items));
    cart_item_count = 0;
    total_bill = 0;
    discount_percent = 0;
    discount_amount  = 0;
    gtk_list_store_clear(cart_store);
    gtk_entry_set_text(GTK_ENTRY(coupon_entry), "");
    update_total_display();

    // Show tracker, then feedback
    show_order_tracker(order_id);
    show_feedback_dialog(order_id);

    // Update analytics labels
    if (analytics_orders_lbl && analytics_revenue_lbl && analytics_avg_lbl) {
        int total_ords = get_total_orders();
        int total_rev  = get_total_revenue();
        int avg        = total_ords > 0 ? total_rev / total_ords : 0;
        char buf[64];
        sprintf(buf, "%d", total_ords); gtk_label_set_text(GTK_LABEL(analytics_orders_lbl), buf);
        sprintf(buf, "Rs. %d", total_rev); gtk_label_set_text(GTK_LABEL(analytics_revenue_lbl), buf);
        sprintf(buf, "Rs. %d", avg); gtk_label_set_text(GTK_LABEL(analytics_avg_lbl), buf);
    }
}

// ==================== SEARCH & FILTER ====================
void on_search_changed(GtkEditable *editable, gpointer user_data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    refresh_menu(text);
}

void on_category_click(GtkWidget *btn, gpointer data) {
    current_category = (char *)data;
    const char *search_text = gtk_entry_get_text(GTK_ENTRY(search_entry));
    refresh_menu(search_text);
}

// ==================== FAVORITES TOGGLE ====================
typedef struct { int inv_idx; GtkWidget *fav_btn; } FavData;

void on_toggle_favorite(GtkWidget *btn, gpointer data) {
    FavData *fd = (FavData *)data;
    toggle_favorite(inventory[fd->inv_idx].name);
    if (is_favorite(inventory[fd->inv_idx].name)) {
        gtk_button_set_label(GTK_BUTTON(fd->fav_btn), "? Saved");
        show_notification("Added to favorites!", "#e21b70");
    } else {
        gtk_button_set_label(GTK_BUTTON(fd->fav_btn), "? Favorite");
        show_notification("Removed from favorites", "#888888");
    }
}

// ==================== MENU REFRESH ====================
void refresh_menu(const char *search_query) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu_grid));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    int count = 0;
    for (int i = 0; i < 20; i++) {
        int cat_match = (strcmp(current_category, "All Items") == 0 ||
                         strcmp(inventory[i].category, current_category) == 0);
        int fav_match = (strcmp(current_category, "? Favorites") == 0 && is_favorite(inventory[i].name));
        int search_match = (search_query == NULL || strlen(search_query) == 0 ||
                            g_strrstr(g_ascii_strdown(inventory[i].name, -1),
                                      g_ascii_strdown(search_query, -1)));

        if ((cat_match || fav_match) && search_match) {
            GtkWidget *card = gtk_button_new();
            gtk_style_context_add_class(gtk_widget_get_style_context(card), "product-card");
            gtk_widget_set_size_request(card, 200, 280);

            GtkWidget *cv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            GtkWidget *img = create_product_image(inventory[i].img_path);

            GtkWidget *name = gtk_label_new(inventory[i].name);
            gtk_label_set_line_wrap(GTK_LABEL(name), TRUE);
            gtk_label_set_max_width_chars(GTK_LABEL(name), 20);
            gtk_label_set_justify(GTK_LABEL(name), GTK_JUSTIFY_CENTER);

            char p[32];
            sprintf(p, "Rs. %d", inventory[i].price);
            GtkWidget *price = gtk_label_new(p);
            gtk_style_context_add_class(gtk_widget_get_style_context(price), "price-tag");

            // Favorite button
            FavData *fd = g_new(FavData, 1);
            fd->inv_idx = i;
            GtkWidget *fav_btn = gtk_button_new_with_label(
                is_favorite(inventory[i].name) ? "? Saved" : "? Favorite");
            fd->fav_btn = fav_btn;
            gtk_style_context_add_class(gtk_widget_get_style_context(fav_btn), "btn-fav");
            g_signal_connect(fav_btn, "clicked", G_CALLBACK(on_toggle_favorite), fd);

            gtk_box_pack_start(GTK_BOX(cv), img,     TRUE,  TRUE,  0);
            gtk_box_pack_start(GTK_BOX(cv), name,    FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(cv), price,   FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(cv), fav_btn, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(card), cv);
            g_signal_connect(card, "clicked", G_CALLBACK(on_add_to_cart), GINT_TO_POINTER(i));
            gtk_grid_attach(GTK_GRID(menu_grid), card, count % 4, count / 4, 1, 1);
            count++;
        }
    }

    if (count == 0) {
        GtkWidget *empty_lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(empty_lbl),
            "<span size='x-large' color='#ccc'>?? No items found</span>");
        gtk_grid_attach(GTK_GRID(menu_grid), empty_lbl, 0, 0, 1, 1);
    }

    gtk_widget_show_all(menu_grid);
}

// ==================== NAVIGATION ====================
void on_nav_click(GtkWidget *b, gpointer page_id) {
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), (const char *)page_id);
}

// ==================== PROFILE HANDLERS ====================
void on_image_upload_clicked(GtkWidget *btn, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Profile Picture", NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open",   GTK_RESPONSE_ACCEPT,
        NULL);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image Files");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(filename, 120, 120, TRUE, NULL);
        if (pb) gtk_image_set_from_pixbuf(GTK_IMAGE(profile_pixbuf_widget), pb);
        g_free(filename);
        show_notification("? Profile photo updated!", "#27ae60");
    }
    gtk_widget_destroy(dialog);
}

void on_profile_update(GtkWidget *btn, gpointer data) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Profile Updated!");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
        "Your profile settings have been saved.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    show_notification("? Profile saved successfully!", "#27ae60");
}

void on_dark_mode_toggled(GtkSwitch *sw, gboolean state, gpointer user_data) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(user_data));
    if (state) {
        gtk_style_context_add_class(ctx, "dark-mode");
        show_notification("?? Dark mode enabled", "#333");
    } else {
        gtk_style_context_remove_class(ctx, "dark-mode");
        show_notification("? Light mode enabled", "#f39c12");
    }
}

// ==================== LOGIN SCREEN ====================
GtkWidget *login_stack;

void on_login_submit(GtkWidget *btn, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *user = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    if (verify_login(user, pass)) {
        strncpy(logged_in_user, user, 63);
        gtk_stack_set_visible_child_name(GTK_STACK(login_stack), "app");
        show_notification("?? Welcome back, Burhan Panda!", "#e21b70");
    } else {
        GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Login Failed");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(err),
            "Invalid username or password!\nDefault: admin / 1234");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
    }
}

GtkWidget* create_login_page() {
    GtkWidget *bg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(center_box, TRUE);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "login-card");
    gtk_widget_set_size_request(card, 400, -1);

    GtkWidget *logo = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(logo),
        "<span size='xx-large' weight='heavy' color='#e21b70'>?? Burhan Panda</span>");

    GtkWidget *sub = gtk_label_new("Food Management System - v2.0");

    GtkWidget *user_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry), "?? Username");
    gtk_style_context_add_class(gtk_widget_get_style_context(user_entry), "login-entry");

    GtkWidget *pass_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "?? Password");
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(pass_entry), "login-entry");

    static GtkWidget *entries[2];
    entries[0] = user_entry;
    entries[1] = pass_entry;

    GtkWidget *login_btn = gtk_button_new_with_label("Login ?");
    gtk_style_context_add_class(gtk_widget_get_style_context(login_btn), "login-btn");
    g_signal_connect(login_btn, "clicked", G_CALLBACK(on_login_submit), entries);
    g_signal_connect(pass_entry, "activate", G_CALLBACK(on_login_submit), entries);

    GtkWidget *hint = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(hint), "<span color='#aaa' size='small'>Default: admin / 1234</span>");

    gtk_box_pack_start(GTK_BOX(card), logo,      FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(card), sub,        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), user_entry, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(card), pass_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), login_btn,  FALSE, FALSE, 15);
    gtk_box_pack_start(GTK_BOX(card), hint,        FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(center_box), card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bg), center_box, TRUE, TRUE, 0);
    return bg;
}

// ==================== PAGE CREATORS ====================

GtkWidget* create_dashboard() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *vbox   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    // Category filter bar
    GtkWidget *cat_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cat_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    GtkWidget *cat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    const char *cats[] = {"All Items","Burgers","Pizza","Pasta","BBQ","Rice","Drinks","Desserts","? Favorites"};
    for (int i = 0; i < 9; i++) {
        GtkWidget *btn = gtk_button_new_with_label(cats[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "category-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_category_click), (gpointer)cats[i]);
        gtk_box_pack_start(GTK_BOX(cat_box), btn, FALSE, FALSE, 0);
    }
    gtk_container_add(GTK_CONTAINER(cat_scroll), cat_box);
    gtk_box_pack_start(GTK_BOX(vbox), cat_scroll, FALSE, FALSE, 0);

    menu_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(menu_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(menu_grid), 20);
    refresh_menu("");
    gtk_box_pack_start(GTK_BOX(vbox), menu_grid, TRUE, TRUE, 0);
    return scroll;
}

GtkWidget* create_orders_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(box), 30);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold' color='#D70F64'>?? Order History</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    order_history_store = gtk_list_store_new(4,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(order_history_store));
    GtkCellRenderer *r = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Order ID",  r, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Date",      r, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Status",    r, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Payment",   r, "text", 3, NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), tree);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *track_btn = gtk_button_new_with_label("?? Track Latest Order");
    gtk_style_context_add_class(gtk_widget_get_style_context(track_btn), "btn-success");
    g_signal_connect_swapped(track_btn, "clicked",
        G_CALLBACK(show_order_tracker), (gpointer)"#LATEST");
    gtk_box_pack_start(GTK_BOX(btn_row), track_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), title,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), scroll,  TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(box), btn_row, FALSE, FALSE, 0);
    return box;
}

GtkWidget* create_analytics_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 30);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold' color='#D70F64'>?? Analytics Dashboard</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    // Stats cards row
    GtkWidget *stats_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);

    // Card 1: Total Orders
    GtkWidget *card1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(card1), "stat-card");
    gtk_widget_set_size_request(card1, 180, 120);
    GtkWidget *c1_icon = gtk_label_new("??");
    analytics_orders_lbl = gtk_label_new("0");
    gtk_style_context_add_class(gtk_widget_get_style_context(analytics_orders_lbl), "stat-number");
    GtkWidget *c1_lbl = gtk_label_new("Total Orders");
    gtk_style_context_add_class(gtk_widget_get_style_context(c1_lbl), "stat-label");
    gtk_box_pack_start(GTK_BOX(card1), c1_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card1), analytics_orders_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card1), c1_lbl, FALSE, FALSE, 0);

    // Card 2: Total Revenue
    GtkWidget *card2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(card2), "stat-card");
    gtk_widget_set_size_request(card2, 180, 120);
    GtkWidget *c2_icon = gtk_label_new("??");
    analytics_revenue_lbl = gtk_label_new("Rs. 0");
    gtk_style_context_add_class(gtk_widget_get_style_context(analytics_revenue_lbl), "stat-number");
    GtkWidget *c2_lbl = gtk_label_new("Total Revenue");
    gtk_style_context_add_class(gtk_widget_get_style_context(c2_lbl), "stat-label");
    gtk_box_pack_start(GTK_BOX(card2), c2_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card2), analytics_revenue_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card2), c2_lbl, FALSE, FALSE, 0);

    // Card 3: Average Order
    GtkWidget *card3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(card3), "stat-card");
    gtk_widget_set_size_request(card3, 180, 120);
    GtkWidget *c3_icon = gtk_label_new("??");
    analytics_avg_lbl = gtk_label_new("Rs. 0");
    gtk_style_context_add_class(gtk_widget_get_style_context(analytics_avg_lbl), "stat-number");
    GtkWidget *c3_lbl = gtk_label_new("Average Order");
    gtk_style_context_add_class(gtk_widget_get_style_context(c3_lbl), "stat-label");
    gtk_box_pack_start(GTK_BOX(card3), c3_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card3), analytics_avg_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card3), c3_lbl, FALSE, FALSE, 0);

    // Card 4: Menu Items
    GtkWidget *card4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(card4), "stat-card");
    gtk_widget_set_size_request(card4, 180, 120);
    GtkWidget *c4_icon = gtk_label_new("??");
    GtkWidget *c4_num  = gtk_label_new("20");
    gtk_style_context_add_class(gtk_widget_get_style_context(c4_num), "stat-number");
    GtkWidget *c4_lbl  = gtk_label_new("Menu Items");
    gtk_style_context_add_class(gtk_widget_get_style_context(c4_lbl), "stat-label");
    gtk_box_pack_start(GTK_BOX(card4), c4_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card4), c4_num, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card4), c4_lbl, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(stats_row), card1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stats_row), card2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stats_row), card3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stats_row), card4, FALSE, FALSE, 0);

    // Popular categories bar chart (simple text-based)
    GtkWidget *chart_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(chart_title), "<b>Category Distribution</b>");
    gtk_widget_set_halign(chart_title, GTK_ALIGN_START);

    GtkWidget *chart_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(chart_box), "stat-card");
    const char *cats[]  = {"Burgers","Pizza","Pasta","BBQ","Rice","Drinks","Desserts"};
    int percentages[] = {20, 18, 15, 12, 12, 13, 10};
    for (int i = 0; i < 7; i++) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget *cat_lbl = gtk_label_new(cats[i]);
        gtk_widget_set_size_request(cat_lbl, 80, -1);
        gtk_widget_set_halign(cat_lbl, GTK_ALIGN_START);
        GtkWidget *bar = gtk_progress_bar_new();
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar), percentages[i] / 100.0);
        char pct_str[16];
        sprintf(pct_str, "%d%%", percentages[i]);
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(bar), TRUE);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(bar), pct_str);
        gtk_widget_set_hexpand(bar, TRUE);
        gtk_box_pack_start(GTK_BOX(row), cat_lbl, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), bar,     TRUE,  TRUE,  0);
        gtk_box_pack_start(GTK_BOX(chart_box), row, FALSE, FALSE, 0);
    }

    // Refresh stats button
    GtkWidget *refresh_btn = gtk_button_new_with_label("?? Refresh Stats");
    gtk_style_context_add_class(gtk_widget_get_style_context(refresh_btn), "btn-success");

    gtk_box_pack_start(GTK_BOX(box), title,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), stats_row,   FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), chart_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), chart_box,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), refresh_btn, FALSE, FALSE, 0);

    // Load initial stats
    int total_ords = get_total_orders();
    int total_rev  = get_total_revenue();
    int avg        = total_ords > 0 ? total_rev / total_ords : 0;
    char buf[64];
    sprintf(buf, "%d", total_ords); gtk_label_set_text(GTK_LABEL(analytics_orders_lbl), buf);
    sprintf(buf, "Rs. %d", total_rev); gtk_label_set_text(GTK_LABEL(analytics_revenue_lbl), buf);
    sprintf(buf, "Rs. %d", avg); gtk_label_set_text(GTK_LABEL(analytics_avg_lbl), buf);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), box);
    return scroll;
}

GtkWidget* create_profile_page() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(box), 40);
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold' color='#D70F64'>?? Profile Settings</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    profile_pixbuf_widget = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);

    GtkWidget *img_btn = gtk_button_new_with_label("?? Upload Photo");
    gtk_style_context_add_class(gtk_widget_get_style_context(img_btn), "category-btn");
    g_signal_connect(img_btn, "clicked", G_CALLBACK(on_image_upload_clicked), NULL);

    GtkWidget *name_lbl  = gtk_label_new("Full Name");
    gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);
    GtkWidget *name      = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Enter your full name");
    gtk_entry_set_text(GTK_ENTRY(name), "Burhan Ahmed");

    GtkWidget *email_lbl = gtk_label_new("Email Address");
    gtk_widget_set_halign(email_lbl, GTK_ALIGN_START);
    GtkWidget *email     = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(email), "Enter your email");

    GtkWidget *phone_lbl = gtk_label_new("Phone Number");
    gtk_widget_set_halign(phone_lbl, GTK_ALIGN_START);
    GtkWidget *phone     = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(phone), "+92 300 0000000");

    GtkWidget *address_lbl = gtk_label_new("Delivery Address");
    gtk_widget_set_halign(address_lbl, GTK_ALIGN_START);
    GtkWidget *address   = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(address), "Enter delivery address");

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    GtkWidget *pwd_lbl  = gtk_label_new("Change Password");
    gtk_widget_set_halign(pwd_lbl, GTK_ALIGN_START);
    GtkWidget *old_pwd  = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(old_pwd), "Current password");
    gtk_entry_set_visibility(GTK_ENTRY(old_pwd), FALSE);
    GtkWidget *new_pwd  = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(new_pwd), "New password");
    gtk_entry_set_visibility(GTK_ENTRY(new_pwd), FALSE);

    GtkWidget *save = gtk_button_new_with_label("?? Save Profile");
    gtk_style_context_add_class(gtk_widget_get_style_context(save), "btn-checkout");
    g_signal_connect(save, "clicked", G_CALLBACK(on_profile_update), NULL);

    gtk_box_pack_start(GTK_BOX(box), title,       FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), profile_pixbuf_widget, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), img_btn,     FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), name_lbl,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), name,         FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), email_lbl,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), email,        FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), phone_lbl,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), phone,        FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), address_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), address,      FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), sep,          FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), pwd_lbl,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), old_pwd,      FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), new_pwd,      FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), save,         FALSE, FALSE, 15);
    return scroll;
}

GtkWidget* create_settings_page(GtkWidget *win) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box), 40);
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold' color='#D70F64'>? App Settings</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    // Dark mode
    GtkWidget *sw_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *sw_lbl = gtk_label_new("?? Enable Dark Mode");
    gtk_widget_set_hexpand(sw_lbl, TRUE);
    gtk_widget_set_halign(sw_lbl, GTK_ALIGN_START);
    GtkWidget *sw = gtk_switch_new();
    g_signal_connect(sw, "state-set", G_CALLBACK(on_dark_mode_toggled), win);
    gtk_box_pack_start(GTK_BOX(sw_box), sw_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sw_box), sw, FALSE, FALSE, 0);

    // Notification toggle
    GtkWidget *notif_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *notif_sw_lbl = gtk_label_new("?? Show Notifications");
    gtk_widget_set_hexpand(notif_sw_lbl, TRUE);
    gtk_widget_set_halign(notif_sw_lbl, GTK_ALIGN_START);
    GtkWidget *notif_sw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(notif_sw), TRUE);
    gtk_box_pack_start(GTK_BOX(notif_box), notif_sw_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(notif_box), notif_sw, FALSE, FALSE, 0);

    // About section
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *about_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(about_lbl),
        "<b>About Burhan Panda v2.0</b>\n"
        "<span color='#888'>Built with GTK3 + SQLite3\n"
        "Features: Login, Cart, Coupons, Receipts,\n"
        "Analytics, Favorites, Order Tracking\n"
        "© 2025 Burhan Panda Systems</span>");
    gtk_widget_set_halign(about_lbl, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(box), title,      FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), sw_box,     FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), notif_box,  FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), sep,         FALSE, FALSE, 15);
    gtk_box_pack_start(GTK_BOX(box), about_lbl,  FALSE, FALSE, 5);
    return scroll;
}

// ==================== MAIN UI ASSEMBLY ====================

GtkWidget* build_app_ui() {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // ---- TOP HEADER ----
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "top-header");

    GtkWidget *logo = gtk_label_new("Burhan Panda ??");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "logo-text");
    gtk_box_pack_start(GTK_BOX(header), logo, FALSE, FALSE, 0);

    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(header), spacer, TRUE, TRUE, 0);

    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "?? Search for foods...");
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "search-bar");
    gtk_widget_set_size_request(search_entry, 350, -1);
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(header), search_entry, FALSE, FALSE, 0);

    // User badge
    char user_badge[80];
    sprintf(user_badge, "?? %s", logged_in_user);
    GtkWidget *user_lbl = gtk_label_new(user_badge);
    GtkStyleContext *ulc = gtk_widget_get_style_context(user_lbl);
    gtk_style_context_add_class(ulc, "logo-text");
    gtk_box_pack_start(GTK_BOX(header), user_lbl, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    // ---- NOTIFICATION REVEALER ----
    notif_revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(notif_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(GTK_REVEALER(notif_revealer), 300);
    notif_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(notif_label), "notif-text");
    GtkWidget *notif_box_inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(notif_box_inner), "notification-bar");
    gtk_box_pack_start(GTK_BOX(notif_box_inner), notif_label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(notif_revealer), notif_box_inner);
    gtk_box_pack_start(GTK_BOX(root), notif_revealer, FALSE, FALSE, 0);

    // ---- MAIN BODY ----
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(root), main_hbox, TRUE, TRUE, 0);

    // Sidebar
    GtkWidget *side = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(side), "sidebar");
    gtk_container_set_border_width(GTK_CONTAINER(side), 10);

    const char *nav_items[] = {"?? Dashboard", "?? Orders", "?? Analytics", "?? Profile", "? Settings"};
    const char *nav_ids[]   = {"dash", "orders", "analytics", "prof", "set"};
    for (int i = 0; i < 5; i++) {
        GtkWidget *b = gtk_button_new_with_label(nav_items[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(b), "nav-btn");
        g_signal_connect(b, "clicked", G_CALLBACK(on_nav_click), (gpointer)nav_ids[i]);
        gtk_box_pack_start(GTK_BOX(side), b, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(main_hbox), side, FALSE, FALSE, 0);

    // Page stack
    main_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(main_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(main_stack), 200);
    gtk_stack_add_named(GTK_STACK(main_stack), create_dashboard(),         "dash");
    gtk_stack_add_named(GTK_STACK(main_stack), create_orders_page(),       "orders");
    gtk_stack_add_named(GTK_STACK(main_stack), create_analytics_page(),    "analytics");
    gtk_stack_add_named(GTK_STACK(main_stack), create_profile_page(),      "prof");
    gtk_stack_add_named(GTK_STACK(main_stack), create_settings_page(main_window), "set");
    gtk_box_pack_start(GTK_BOX(main_hbox), main_stack, TRUE, TRUE, 0);

    // ---- CART PANEL ----
    GtkWidget *cart = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(cart), "cart-panel");
    gtk_widget_set_size_request(cart, 320, -1);
    gtk_container_set_border_width(GTK_CONTAINER(cart), 15);

    GtkWidget *basket_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(basket_label),
        "<span size='x-large' weight='heavy'>Your Basket ??</span>");
    gtk_widget_set_halign(basket_label, GTK_ALIGN_START);

    // Order type selector
    GtkWidget *otype_lbl = gtk_label_new("Order Type:");
    gtk_widget_set_halign(otype_lbl, GTK_ALIGN_START);
    GtkWidget *otype_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    order_type_dine_btn = gtk_button_new_with_label("?? Dine-In");
    order_type_take_btn = gtk_button_new_with_label("?? Takeaway");
    gtk_style_context_add_class(gtk_widget_get_style_context(order_type_dine_btn), "category-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(order_type_take_btn), "category-btn");
    g_signal_connect(order_type_dine_btn, "clicked", G_CALLBACK(on_select_dine), NULL);
    g_signal_connect(order_type_take_btn, "clicked", G_CALLBACK(on_select_take), NULL);
    gtk_box_pack_start(GTK_BOX(otype_row), order_type_dine_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(otype_row), order_type_take_btn, TRUE, TRUE, 0);

    // Cart list (Name | Qty | Price)
    cart_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    cart_view  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cart_store));
    g_signal_connect(cart_view, "row-activated", G_CALLBACK(on_remove_item), NULL);
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cart_view), -1, "Item",  r, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cart_view), -1, "Qty",   r, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cart_view), -1, "Total", r, "text", 2, NULL);

    GtkWidget *cs = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(cs, -1, 200);
    gtk_container_add(GTK_CONTAINER(cs), cart_view);

    GtkWidget *hint_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(hint_lbl),
        "<span size='small' color='#aaa'>Double-click item to remove</span>");

    // Coupon section
    GtkWidget *coupon_lbl = gtk_label_new("?? Coupon Code:");
    gtk_widget_set_halign(coupon_lbl, GTK_ALIGN_START);
    GtkWidget *coupon_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    coupon_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(coupon_entry), "e.g. PANDA10");
    gtk_widget_set_hexpand(coupon_entry, TRUE);
    GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
    gtk_style_context_add_class(gtk_widget_get_style_context(apply_btn), "btn-success");
    GtkWidget *rem_btn = gtk_button_new_with_label("?");
    gtk_style_context_add_class(gtk_widget_get_style_context(rem_btn), "btn-danger");
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_apply_coupon), NULL);
    g_signal_connect(rem_btn,   "clicked", G_CALLBACK(on_remove_coupon), NULL);
    gtk_box_pack_start(GTK_BOX(coupon_row), coupon_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(coupon_row), apply_btn,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(coupon_row), rem_btn,      FALSE, FALSE, 0);

    // Discount label (hidden initially)
    discount_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(discount_label), "discount-lbl");
    gtk_widget_hide(discount_label);

    // Total
    total_label = gtk_label_new("Rs. 0");
    gtk_style_context_add_class(gtk_widget_get_style_context(total_label), "total-lbl");
    gtk_widget_set_halign(total_label, GTK_ALIGN_CENTER);

    // Checkout button
    GtkWidget *checkout_btn = gtk_button_new_with_label("?? Checkout");
    gtk_style_context_add_class(gtk_widget_get_style_context(checkout_btn), "btn-checkout");
    g_signal_connect(checkout_btn, "clicked", G_CALLBACK(on_checkout), NULL);

    gtk_box_pack_start(GTK_BOX(cart), basket_label,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), otype_lbl,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), otype_row,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), cs,             TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(cart), hint_lbl,      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), coupon_lbl,    FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(cart), coupon_row,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), discount_label,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), total_label,   FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(cart), checkout_btn,  FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_hbox), cart, FALSE, FALSE, 0);
    return root;
}

// ==================== MAIN ====================
int main(int argc, char *argv[]) {
    srand(time(NULL));
    gtk_init(&argc, &argv);

    init_database();

    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, ULTIMATE_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(cp), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Main window
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Burhan Panda - Food Management v2.0");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Login stack (login screen ? app screen)
    login_stack = gtk_stack_new();
    gtk_stack_add_named(GTK_STACK(login_stack), create_login_page(), "login");
    gtk_stack_add_named(GTK_STACK(login_stack), build_app_ui(),      "app");
    gtk_stack_set_visible_child_name(GTK_STACK(login_stack), "login");
    gtk_container_add(GTK_CONTAINER(main_window), login_stack);

    gtk_widget_show_all(main_window);
    gtk_main();

    sqlite3_close(db);
    return 0;
}
