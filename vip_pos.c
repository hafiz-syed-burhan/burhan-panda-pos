// ============================================================
//   BURHAN PANDA FOOD MANAGEMENT SYSTEM  v3.0
//   FIXED VERSION - All bugs resolved, fully responsive
//   Works correctly on laptop and projector screens
// ============================================================
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

#define INVENTORY_SIZE 20
#define CART_MAX       100

// ==================== DATABASE ====================
sqlite3 *db;

void init_database() {
    if (sqlite3_open("burhan_panda.db", &db) != SQLITE_OK) {
        fprintf(stderr, "DB Error: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS orders("
        "id TEXT, date TEXT, status TEXT, total INT,"
        "payment_method TEXT, order_type TEXT, feedback INT);",
        0, 0, 0);
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS favorites(item_name TEXT UNIQUE);",
        0, 0, 0);
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS users("
        "username TEXT UNIQUE, password TEXT, fullname TEXT, email TEXT);",
        0, 0, 0);
    sqlite3_exec(db,
        "INSERT OR IGNORE INTO users VALUES('admin','1234','Burhan Ahmed','admin@panda.pk');",
        0, 0, 0);
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS coupons("
        "code TEXT UNIQUE, discount_percent INT, min_order INT, used INT DEFAULT 0);",
        0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('PANDA10',10,500,0);",  0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('SAVE20',20,1000,0);",  0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('FLAT50',50,2000,0);",  0, 0, 0);
}

void save_order_to_db(const char *id, int total, const char *pay, const char *otype) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO orders VALUES('%s',datetime('now','localtime'),'Preparing',%d,'%s','%s',0);",
        id, total, pay, otype);
    sqlite3_exec(db, sql, 0, 0, 0);
}

int verify_login(const char *user, const char *pass) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT count(*) FROM users WHERE username='%s' AND password='%s';", user, pass);
    sqlite3_stmt *stmt;
    int found = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) found = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return found;
}

int check_coupon(const char *code, int bill) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT discount_percent,min_order FROM coupons WHERE code='%s' AND used=0;", code);
    sqlite3_stmt *stmt;
    int discount = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int pct = sqlite3_column_int(stmt, 0);
            int mn  = sqlite3_column_int(stmt, 1);
            if (bill >= mn) discount = pct;
        }
        sqlite3_finalize(stmt);
    }
    return discount;
}

void mark_coupon_used(const char *code) {
    char sql[128];
    snprintf(sql, sizeof(sql), "UPDATE coupons SET used=1 WHERE code='%s';", code);
    sqlite3_exec(db, sql, 0, 0, 0);
}

void coupon_reset_all() {
    /* Reset all coupons for testing - call from settings if needed */
    sqlite3_exec(db, "UPDATE coupons SET used=0;", 0, 0, 0);
}

int get_total_orders() {
    sqlite3_stmt *stmt;
    int n = 0;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM orders;", -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
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

void toggle_favorite(const char *name) {
    char sql[256];
    sqlite3_stmt *stmt;
    int exists = 0;
    snprintf(sql, sizeof(sql), "SELECT count(*) FROM favorites WHERE item_name='%s';", name);
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (exists) {
        snprintf(sql, sizeof(sql), "DELETE FROM favorites WHERE item_name='%s';", name);
    } else {
        snprintf(sql, sizeof(sql), "INSERT OR IGNORE INTO favorites VALUES('%s');", name);
    }
    sqlite3_exec(db, sql, 0, 0, 0);
}

int is_favorite(const char *name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT count(*) FROM favorites WHERE item_name='%s';", name);
    sqlite3_stmt *stmt;
    int res = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) res = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return res;
}

// ==================== RESPONSIVE SCALE ====================
// Ye value screen resolution ke hisaab se set hoti hai
static double UI_SCALE = 1.0;

void compute_ui_scale() {
    GdkScreen *screen = gdk_screen_get_default();
    int sw = gdk_screen_get_width(screen);
    // Base: 1366px laptop width
    // Projector ya bada screen: scale up
    if      (sw >= 3840) UI_SCALE = 2.2;
    else if (sw >= 2560) UI_SCALE = 1.7;
    else if (sw >= 1920) UI_SCALE = 1.3;
    else if (sw >= 1600) UI_SCALE = 1.15;
    else if (sw >= 1366) UI_SCALE = 1.0;
    else                 UI_SCALE = 0.9;
}

int SC(int val) { return (int)(val * UI_SCALE); }

// ==================== CSS (GTK3 COMPATIBLE) ====================
// GTK3 CSS: font-size MUST be in pt, font-weight MUST be bold/normal
// color on labels: use "label.classname { color: }"
// No box-shadow on regular widgets, no letter-spacing support
// padding works on buttons/entries

static void load_css() {
    char css[8192];

    // Font sizes in pt — properly scaled
    int fs_small   = SC(8);   // ~8pt
    int fs_normal  = SC(10);  // ~10pt
    int fs_medium  = SC(11);  // ~11pt
    int fs_large   = SC(13);  // ~13pt
    int fs_xlarge  = SC(16);  // ~16pt
    int fs_xxlarge = SC(20);  // ~20pt
    int fs_huge    = SC(24);  // ~24pt

    // Border radius in px
    int br_small  = SC(8);
    int br_medium = SC(14);
    int br_large  = SC(20);

    snprintf(css, sizeof(css),
        /* === GLOBAL === */
        "* { font-family: 'Ubuntu', 'DejaVu Sans', sans-serif; }"

        /* === WINDOW === */
        "window { background-color: #F4F6F8; }"

        /* === TOP HEADER === */
        ".top-header {"
        "   background-color: #C0175F;"
        "   padding: %dpx %dpx;"
        "}"

        /* === LOGO LABEL === */
        "label.logo-text {"
        "   color: #FFFFFF;"
        "   font-size: %dpt;"
        "   font-weight: bold;"
        "}"

        /* === SEARCH BAR === */
        ".search-bar entry {"
        "   border-radius: %dpx;"
        "   padding: %dpx %dpx;"
        "   font-size: %dpt;"
        "   border: 1px solid #ddd;"
        "   background-color: white;"
        "}"

        /* === USER BADGE === */
        "label.user-badge {"
        "   color: #FFFFFF;"
        "   font-size: %dpt;"
        "   font-weight: bold;"
        "}"

        /* === SIDEBAR === */
        ".sidebar {"
        "   background-color: #FFFFFF;"
        "   border-right: 1px solid #E8E8E8;"
        "   padding: %dpx %dpx;"
        "}"

        /* === NAV BUTTONS === */
        "button.nav-btn {"
        "   background-color: transparent;"
        "   border: none;"
        "   border-radius: %dpx;"
        "   padding: %dpx %dpx;"
        "   font-size: %dpt;"
        "   font-weight: bold;"
        "}"
        "button.nav-btn label { color: #333333; font-weight: bold; font-size: %dpt; }"
        "button.nav-btn:hover { background-color: #C0175F; }"
        "button.nav-btn:hover label { color: #FFFFFF; }"

        /* === CATEGORY BUTTONS === */
        "button.cat-btn {"
        "   background-color: #F0F0F0;"
        "   border: 1px solid #DDDDDD;"
        "   border-radius: %dpx;"
        "   padding: %dpx %dpx;"
        "   font-size: %dpt;"
        "}"
        "button.cat-btn label { color: #333333; font-size: %dpt; }"
        "button.cat-btn:hover { background-color: #C0175F; }"
        "button.cat-btn:hover label { color: #FFFFFF; }"

        /* === PRODUCT CARDS === */
        "button.product-card {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #EEEEEE;"
        "   border-radius: %dpx;"
        "   padding: %dpx;"
        "}"
        "button.product-card:hover { background-color: #FFF0F5; border: 2px solid #C0175F; }"
        "label.price-tag { color: #C0175F; font-size: %dpt; font-weight: bold; }"
        "label.item-name { color: #222222; font-size: %dpt; font-weight: bold; }"

        /* === CART PANEL === */
        ".cart-panel { background-color: #FFFFFF; border-left: 1px solid #EEEEEE; }"
        "label.cart-title { color: #222222; font-size: %dpt; font-weight: bold; }"
        "label.total-lbl  { color: #C0175F; font-size: %dpt; font-weight: bold; }"
        "label.discount-lbl { color: #1E8A44; font-size: %dpt; font-weight: bold; }"

        /* === CHECKOUT BUTTON === */
        "button.btn-checkout {"
        "   background-color: #C0175F;"
        "   border-radius: %dpx;"
        "   padding: %dpx;"
        "   border: none;"
        "}"
        "button.btn-checkout label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }"
        "button.btn-checkout:hover { background-color: #A01050; }"

        /* === SUCCESS BUTTON === */
        "button.btn-success {"
        "   background-color: #1E8A44;"
        "   border-radius: %dpx;"
        "   padding: %dpx %dpx;"
        "   border: none;"
        "}"
        "button.btn-success label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }"

        /* === DANGER BUTTON === */
        "button.btn-danger {"
        "   background-color: #C0392B;"
        "   border-radius: %dpx;"
        "   padding: %dpx %dpx;"
        "   border: none;"
        "}"
        "button.btn-danger label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }"

        /* === FAVORITE BUTTON === */
        "button.btn-fav {"
        "   background-color: transparent;"
        "   border: 1px solid #C0175F;"
        "   border-radius: %dpx;"
        "   padding: 2px 6px;"
        "}"
        "button.btn-fav label { color: #C0175F; font-size: %dpt; }"

        /* === NOTIFICATION BAR === */
        ".notif-box {"
        "   border-radius: %dpx;"
        "   padding: %dpx %dpx;"
        "   margin: %dpx;"
        "}"
        "label.notif-text { color: #FFFFFF; font-size: %dpt; font-weight: bold; }"

        /* === STAT CARDS (Analytics) === */
        ".stat-card {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #EEEEEE;"
        "   border-radius: %dpx;"
        "   padding: %dpx;"
        "}"
        "label.stat-number { color: #C0175F; font-size: %dpt; font-weight: bold; }"
        "label.stat-label  { color: #888888; font-size: %dpt; }"

        /* === ORDER TRACKER STEPS === */
        "label.step-done   { background-color: #1E8A44; color: white; border-radius: %dpx; padding: 3px 10px; font-size: %dpt; font-weight: bold; }"
        "label.step-active { background-color: #C0175F; color: white; border-radius: %dpx; padding: 3px 10px; font-size: %dpt; font-weight: bold; }"
        "label.step-todo   { background-color: #DDDDDD; color: #888;  border-radius: %dpx; padding: 3px 10px; font-size: %dpt; }"

        /* === LOGIN PAGE === */
        ".login-card {"
        "   background-color: #FFFFFF;"
        "   border-radius: %dpx;"
        "   padding: %dpx;"
        "}"
        "label.login-title { color: #C0175F; font-size: %dpt; font-weight: bold; }"
        "label.login-sub   { color: #888888; font-size: %dpt; }"
        "label.login-hint  { color: #AAAAAA; font-size: %dpt; }"
        ".login-entry entry { border-radius: %dpx; padding: %dpx; font-size: %dpt; border: 2px solid #EEEEEE; }"
        "button.login-btn {"
        "   background-color: #C0175F;"
        "   border-radius: %dpx;"
        "   padding: %dpx;"
        "   border: none;"
        "}"
        "button.login-btn label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }"

        /* === TREEVIEW === */
        "treeview { background-color: #FAFAFA; color: #222222; font-size: %dpt; }"
        "treeview:selected { background-color: #C0175F; color: white; }"
        "treeview header button { background-color: #F8F8F8; }"
        "treeview header button label { color: #C0175F; font-size: %dpt; font-weight: bold; }"

        /* === PROGRESS BAR === */
        "progressbar trough { background-color: #EEEEEE; border-radius: %dpx; min-height: %dpx; }"
        "progressbar progress { background-color: #C0175F; border-radius: %dpx; }",

        /* header padding */     SC(10), SC(30),
        /* logo font */          fs_xlarge,
        /* search border-r */    SC(20),
        /* search padding */     SC(6), SC(14),
        /* search font */        fs_normal,
        /* user badge font */    fs_normal,
        /* sidebar padding */    SC(15), SC(8),
        /* nav btn border-r */   br_medium,
        /* nav btn padding */    SC(10), SC(16),
        /* nav btn font */       fs_medium,
        /* nav label font */     fs_medium,
        /* cat btn border-r */   SC(16),
        /* cat btn padding */    SC(6), SC(14),
        /* cat btn font */       fs_normal,
        /* cat label font */     fs_normal,
        /* card border-r */      br_large,
        /* card padding */       SC(12),
        /* price font */         fs_medium,
        /* name font */          fs_normal,
        /* cart title font */    fs_xlarge,
        /* total font */         fs_xxlarge,
        /* discount font */      fs_normal,
        /* checkout border-r */  br_medium,
        /* checkout padding */   SC(14),
        /* checkout font */      fs_large,
        /* success border-r */   br_small,
        /* success padding */    SC(6), SC(12),
        /* success font */       fs_normal,
        /* danger border-r */    br_small,
        /* danger padding */     SC(6), SC(12),
        /* danger font */        fs_normal,
        /* fav border-r */       SC(14),
        /* fav font */           fs_small,
        /* notif border-r */     br_small,
        /* notif padding */      SC(8), SC(16),
        /* notif margin */       SC(4),
        /* notif font */         fs_normal,
        /* stat card border-r */ br_large,
        /* stat card padding */  SC(18),
        /* stat number font */   fs_xxlarge,
        /* stat label font */    fs_normal,
        /* step-done br,font */  SC(12), fs_small,
        /* step-active br,font */SC(12), fs_small,
        /* step-todo br,font */  SC(12), fs_small,
        /* login card br */      br_large,
        /* login card pad */     SC(35),
        /* login title */        fs_xxlarge,
        /* login sub */          fs_normal,
        /* login hint */         fs_small,
        /* login entry br */     br_small,
        /* login entry pad */    SC(10),
        /* login entry font */   fs_medium,
        /* login btn br */       br_medium,
        /* login btn pad */      SC(12),
        /* login btn font */     fs_large,
        /* tree font */          fs_normal,
        /* tree hdr font */      fs_normal,
        /* progress br,h,br */   SC(4), SC(14), SC(4)
    );

    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;
    gtk_css_provider_load_from_data(provider, css, -1, &error);
    if (error) {
        fprintf(stderr, "CSS Error: %s\n", error->message);
        g_error_free(error);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

// ==================== DATA STRUCTURES ====================
typedef struct {
    const char *name;
    int         price;
    const char *category;
    const char *img_path;
} FoodItem;

typedef struct {
    char name[128];
    int  price;
    int  quantity;
} CartItem;

static FoodItem inventory[INVENTORY_SIZE] = {
    {"Classic Beef Burger",  750,  "Burgers",  "images/burger.png"},
    {"Zinger Combo",         950,  "Burgers",  "images/zinger.png"},
    {"Double Cheese Patty",  850,  "Burgers",  "images/double.png"},
    {"Pepperoni Pizza L",    1500, "Pizza",    "images/pizza.png"},
    {"Margherita M",         1100, "Pizza",    "images/margherita.png"},
    {"Fajita Passion",       1300, "Pizza",    "images/fajita.png"},
    {"Alfredo Pasta",        850,  "Pasta",    "images/pasta.png"},
    {"Arrabiata Pasta",      780,  "Pasta",    "images/arrabiata.png"},
    {"Mac n Cheese",         700,  "Pasta",    "images/mac.png"},
    {"Chicken Tikka",        450,  "BBQ",      "images/tikka.png"},
    {"Beef Seekh Kabab",     550,  "BBQ",      "images/kabab.png"},
    {"Malai Boti",           600,  "BBQ",      "images/malai.png"},
    {"Beef Biryani",         650,  "Rice",     "images/biryani.png"},
    {"Chicken Pulao",        500,  "Rice",     "images/pulao.png"},
    {"Iced Caramel Latte",   550,  "Drinks",   "images/coffee.png"},
    {"Coca Cola 500ml",      150,  "Drinks",   "images/coke.png"},
    {"Fresh Lime",           200,  "Drinks",   "images/lime.png"},
    {"Chocolate Lava Cake",  450,  "Desserts", "images/cake.png"},
    {"New York Cheesecake",  600,  "Desserts", "images/cheesecake.png"},
    {"Gulab Jamun (2pc)",    250,  "Desserts", "images/jamun.png"}
};

// ==================== GLOBAL STATE ====================
GtkWidget    *main_window;
GtkWidget    *login_stack;          // top-level: "login" vs "app"
GtkWidget    *main_stack;           // inner pages
GtkWidget    *menu_grid;
GtkWidget    *cart_view;
GtkWidget    *total_label;
GtkWidget    *discount_label;
GtkWidget    *coupon_entry;
GtkWidget    *search_entry;
GtkWidget    *notif_box_widget;     // the colored notification box
GtkWidget    *notif_label;
GtkWidget    *notif_revealer;
GtkWidget    *profile_pixbuf_widget;
GtkWidget    *analytics_orders_lbl;
GtkWidget    *analytics_revenue_lbl;
GtkWidget    *analytics_avg_lbl;
GtkListStore *cart_store;
GtkListStore *order_history_store;

CartItem  cart_items[CART_MAX];
int       cart_item_count  = 0;
int       total_bill       = 0;
int       discount_percent = 0;
int       discount_amount  = 0;

// Current category: stored as index into cats array (safe, no dangling ptr)
static const char *CAT_NAMES[] = {
    "All Items","Burgers","Pizza","Pasta","BBQ","Rice","Drinks","Desserts","Favorites"
};
static int current_cat_idx = 0;   // 0 = All Items

char current_order_type[32] = "Dine-In";
char current_payment[32]    = "Cash";
char logged_in_user[64]     = "";
guint notif_timeout_id      = 0;

// ==================== FORWARD DECLARATIONS ====================
void update_total_display(void);
void refresh_cart_view(void);
void refresh_menu(const char *query);

// ==================== NOTIFICATION ====================
static gboolean hide_notif_cb(gpointer data) {
    (void)data;
    gtk_revealer_set_reveal_child(GTK_REVEALER(notif_revealer), FALSE);
    notif_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

// color: e.g. "#27ae60"  or  "#C0175F"
void show_notification(const char *msg, const char *bg_color) {
    if (!notif_label || !notif_revealer) return;
    gtk_label_set_text(GTK_LABEL(notif_label), msg);

    // Dynamically recolor the notification box
    char css[256];
    snprintf(css, sizeof(css),
        ".notif-box { background-color: %s; border-radius: %dpx; padding: %dpx %dpx; margin: %dpx; }",
        bg_color, SC(8), SC(8), SC(16), SC(4));
    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, css, -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(notif_box_widget),
        GTK_STYLE_PROVIDER(cp),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(cp);

    gtk_revealer_set_reveal_child(GTK_REVEALER(notif_revealer), TRUE);
    if (notif_timeout_id) g_source_remove(notif_timeout_id);
    notif_timeout_id = g_timeout_add(3000, hide_notif_cb, NULL);
}

// ==================== IMAGE HELPER ====================
GtkWidget *create_product_image(const char *path) {
    int w = SC(140), h = SC(100);
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, w, h, TRUE, NULL);
    if (pb) {
        GtkWidget *img = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb);
        return img;
    }
    // Fallback: colored placeholder label
    GtkWidget *ph = gtk_label_new("??");
    char markup[64];
    snprintf(markup, sizeof(markup), "<span size='%d000'>??</span>", SC(3));
    gtk_label_set_markup(GTK_LABEL(ph), markup);
    gtk_widget_set_size_request(ph, w, h);
    return ph;
}

// ==================== TOTAL DISPLAY ====================
void update_total_display() {
    discount_amount = (total_bill * discount_percent) / 100;
    int final = total_bill - discount_amount;

    char buf[64];
    snprintf(buf, sizeof(buf), "Rs. %d", final);
    gtk_label_set_text(GTK_LABEL(total_label), buf);

    if (discount_percent > 0) {
        char dbuf[80];
        snprintf(dbuf, sizeof(dbuf), "Coupon: %d%% OFF  (- Rs.%d)", discount_percent, discount_amount);
        gtk_label_set_text(GTK_LABEL(discount_label), dbuf);
        gtk_widget_set_visible(discount_label, TRUE);
    } else {
        gtk_widget_set_visible(discount_label, FALSE);
    }
}

// ==================== CART OPERATIONS ====================
void refresh_cart_view() {
    gtk_list_store_clear(cart_store);
    for (int i = 0; i < cart_item_count; i++) {
        GtkTreeIter it;
        char qty_str[16];
        snprintf(qty_str, sizeof(qty_str), "x%d", cart_items[i].quantity);
        int line = cart_items[i].price * cart_items[i].quantity;
        char price_str[32];
        snprintf(price_str, sizeof(price_str), "Rs.%d", line);
        gtk_list_store_append(cart_store, &it);
        gtk_list_store_set(cart_store, &it,
            0, cart_items[i].name,
            1, qty_str,
            2, price_str,
            -1);
    }
}

void add_to_cart(int inv_idx) {
    // Find existing
    for (int i = 0; i < cart_item_count; i++) {
        if (strcmp(cart_items[i].name, inventory[inv_idx].name) == 0) {
            cart_items[i].quantity++;
            total_bill += inventory[inv_idx].price;
            update_total_display();
            refresh_cart_view();
            return;
        }
    }
    // New entry
    if (cart_item_count < CART_MAX) {
        strncpy(cart_items[cart_item_count].name, inventory[inv_idx].name, 127);
        cart_items[cart_item_count].name[127] = '\0';
        cart_items[cart_item_count].price    = inventory[inv_idx].price;
        cart_items[cart_item_count].quantity = 1;
        cart_item_count++;
        total_bill += inventory[inv_idx].price;
        update_total_display();
        refresh_cart_view();
    }
}

// Double-click removes item from cart
void on_remove_item(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, gpointer ud) {
    (void)col; (void)ud;
    int *indices = gtk_tree_path_get_indices(path);
    int row = indices[0];
    if (row < 0 || row >= cart_item_count) return;

    total_bill -= cart_items[row].price * cart_items[row].quantity;
    if (total_bill < 0) total_bill = 0;
    for (int i = row; i < cart_item_count - 1; i++)
        cart_items[i] = cart_items[i + 1];
    cart_item_count--;

    update_total_display();
    refresh_cart_view();
    show_notification("Item removed from cart", "#C0392B");
}

void on_add_to_cart(GtkWidget *w, gpointer data) {
    (void)w;
    int idx = GPOINTER_TO_INT(data);
    add_to_cart(idx);

    char msg[128];
    snprintf(msg, sizeof(msg), "Added: %s", inventory[idx].name);
    show_notification(msg, "#1E8A44");
}

// ==================== COUPON ====================
void on_apply_coupon(GtkWidget *btn, gpointer d) {
    (void)btn; (void)d;
    const char *code = gtk_entry_get_text(GTK_ENTRY(coupon_entry));
    if (!code || strlen(code) == 0) {
        show_notification("Enter a coupon code first!", "#C0392B"); return;
    }
    if (total_bill == 0) {
        show_notification("Add items to cart first!", "#E67E22"); return;
    }
    int disc = check_coupon(code, total_bill);
    if (disc > 0) {
        discount_percent = disc;
        mark_coupon_used(code);
        update_total_display();
        char msg[64];
        snprintf(msg, sizeof(msg), "Coupon applied! %d%% discount!", disc);
        show_notification(msg, "#1E8A44");
    } else {
        show_notification("Invalid or expired coupon code!", "#C0392B");
    }
}

void on_remove_coupon(GtkWidget *btn, gpointer d) {
    (void)btn; (void)d;
    discount_percent = 0;
    discount_amount  = 0;
    gtk_entry_set_text(GTK_ENTRY(coupon_entry), "");
    update_total_display();
    show_notification("Coupon removed", "#E67E22");
}

// ==================== ORDER TYPE ====================
void on_select_dine(GtkWidget *b, gpointer d) { (void)b;(void)d; strcpy(current_order_type,"Dine-In");  show_notification("Dine-In selected","#2980B9"); }
void on_select_take(GtkWidget *b, gpointer d) { (void)b;(void)d; strcpy(current_order_type,"Takeaway"); show_notification("Takeaway selected","#8E44AD"); }

// ==================== RECEIPT ====================
void generate_receipt(const char *oid, int sub, int disc, int fin, const char *pay) {
    char fn[64];
    snprintf(fn, sizeof(fn), "receipt_%s.txt", oid[0]=='#' ? oid+1 : oid);
    FILE *f = fopen(fn, "w");
    if (!f) return;
    time_t now = time(NULL);
    char ts[64]; strncpy(ts, ctime(&now), 63); ts[63]='\0';
    // Remove trailing newline from ctime
    int tl = strlen(ts); if (tl>0 && ts[tl-1]=='\n') ts[tl-1]='\0';

    fprintf(f, "=========================================\n");
    fprintf(f, "         BURHAN PANDA - RECEIPT          \n");
    fprintf(f, "=========================================\n");
    fprintf(f, "Order ID   : %s\n", oid);
    fprintf(f, "Date/Time  : %s\n", ts);
    fprintf(f, "Order Type : %s\n", current_order_type);
    fprintf(f, "Payment    : %s\n", pay);
    fprintf(f, "-----------------------------------------\n");
    fprintf(f, "%-26s %5s %9s\n", "ITEM", "QTY", "TOTAL");
    fprintf(f, "-----------------------------------------\n");
    for (int i = 0; i < cart_item_count; i++) {
        int line = cart_items[i].price * cart_items[i].quantity;
        fprintf(f, "%-26s %5d %9d\n", cart_items[i].name, cart_items[i].quantity, line);
    }
    fprintf(f, "-----------------------------------------\n");
    fprintf(f, "%-26s %14d\n", "Subtotal (Rs.):", sub);
    if (disc > 0)
        fprintf(f, "%-26s %14d\n", "Discount (Rs.):", -disc);
    fprintf(f, "%-26s %14d\n", "TOTAL PAID (Rs.):", fin);
    fprintf(f, "=========================================\n");
    fprintf(f, "   Thank you for choosing Burhan Panda!\n");
    fprintf(f, "=========================================\n");
    fclose(f);
    printf("[INFO] Receipt saved: %s\n", fn);
}

// ==================== PAYMENT DIALOG ====================
int show_payment_dialog() {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Select Payment Method", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Confirm", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), SC(400), SC(280));

    GtkWidget *ca  = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(12));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(20));

    GtkWidget *ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ttl), "<b>Choose How You Want to Pay:</b>");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    GtkWidget *rb_cash = gtk_radio_button_new_with_label(NULL,                             "Cash on Delivery");
    GtkWidget *rb_card = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "Credit / Debit Card");
    GtkWidget *rb_jazz = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "JazzCash");
    GtkWidget *rb_easy = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "EasyPaisa");

    gtk_box_pack_start(GTK_BOX(box), ttl,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), rb_cash, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), rb_card, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), rb_jazz, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), rb_easy, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(ca),  box,     TRUE,  TRUE,  0);
    gtk_widget_show_all(dlg);

    int resp = gtk_dialog_run(GTK_DIALOG(dlg));
    if (resp == GTK_RESPONSE_OK) {
        if      (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_card))) strcpy(current_payment,"Card");
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_jazz))) strcpy(current_payment,"JazzCash");
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_easy))) strcpy(current_payment,"EasyPaisa");
        else                                                                strcpy(current_payment,"Cash");
    }
    gtk_widget_destroy(dlg);
    return resp;
}

// ==================== FEEDBACK DIALOG ====================
void show_feedback_dialog(const char *order_id) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Rate Your Experience", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Skip", GTK_RESPONSE_CANCEL,
        "_Submit", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), SC(420), SC(280));

    GtkWidget *ca  = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(12));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(20));

    GtkWidget *ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ttl), "<b>How was your order experience?</b>\n"
                                          "<small>Your feedback helps us improve!</small>");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *rl  = gtk_label_new("Rating (1-5):");
    GtkWidget *sp  = gtk_spin_button_new_with_range(1, 5, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), 5);
    GtkWidget *ri  = gtk_label_new("  5 = Excellent, 1 = Poor");
    gtk_box_pack_start(GTK_BOX(row), rl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), sp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), ri, FALSE, FALSE, 0);

    GtkWidget *cl = gtk_label_new("Comments (optional):");
    gtk_widget_set_halign(cl, GTK_ALIGN_START);
    GtkWidget *tv = gtk_text_view_new();
    gtk_widget_set_size_request(tv, SC(350), SC(70));

    gtk_box_pack_start(GTK_BOX(box), ttl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), cl,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), tv,  TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(ca),  box, TRUE,  TRUE,  0);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
        int rating = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(sp));
        char sql[256];
        snprintf(sql, sizeof(sql), "UPDATE orders SET feedback=%d WHERE id='%s';", rating, order_id);
        sqlite3_exec(db, sql, 0, 0, 0);
        show_notification("Thank you for your feedback!", "#1E8A44");
    }
    gtk_widget_destroy(dlg);
}

// ==================== ORDER TRACKER ====================
void show_order_tracker(const char *order_id) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Order Status Tracker", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), SC(560), SC(300));

    GtkWidget *ca  = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(16));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(24));

    // Title
    GtkWidget *ttl = gtk_label_new(NULL);
    char mu[128];
    snprintf(mu, sizeof(mu), "<b>Order %s  |  %s</b>", order_id, current_order_type);
    gtk_label_set_markup(GTK_LABEL(ttl), mu);
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    // Steps row
    GtkWidget *steps = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(6));
    const char *step_labels[] = {"Received","Confirmed","Preparing","Ready","Delivered"};
    // Steps 0,1 = done; step 2 = active; 3,4 = todo
    for (int i = 0; i < 5; i++) {
        GtkWidget *sl = gtk_label_new(step_labels[i]);
        const char *cls = (i < 2) ? "step-done" : (i == 2) ? "step-active" : "step-todo";
        gtk_style_context_add_class(gtk_widget_get_style_context(sl), cls);
        gtk_box_pack_start(GTK_BOX(steps), sl, FALSE, FALSE, 0);
        if (i < 4) {
            GtkWidget *arr = gtk_label_new(" > ");
            gtk_box_pack_start(GTK_BOX(steps), arr, FALSE, FALSE, 0);
        }
    }

    // Progress bar
    GtkWidget *pb = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), 0.4);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(pb), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pb), "Preparing your order...");

    // ETA
    GtkWidget *eta = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(eta), "<b>Estimated Time: 20-30 minutes</b>");

    GtkWidget *inf = gtk_label_new("Our chefs are preparing your food with care!");

    gtk_box_pack_start(GTK_BOX(box), ttl,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), steps, FALSE, FALSE, SC(8));
    gtk_box_pack_start(GTK_BOX(box), pb,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), eta,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), inf,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ca),  box,   TRUE,  TRUE,  0);
    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

// ==================== CHECKOUT ====================
void on_checkout(GtkWidget *btn, gpointer d) {
    (void)btn; (void)d;
    if (total_bill == 0) {
        show_notification("Your cart is empty!", "#C0392B"); return;
    }

    if (show_payment_dialog() == GTK_RESPONSE_CANCEL) return;

    char order_id[16];
    snprintf(order_id, sizeof(order_id), "#BK%04d", 1000 + rand() % 9000);

    int subtotal   = total_bill;
    int disc_amt   = (subtotal * discount_percent) / 100;
    int final      = subtotal - disc_amt;

    save_order_to_db(order_id, final, current_payment, current_order_type);

    // Add to order history list
    if (order_history_store) {
        GtkTreeIter it;
        gtk_list_store_append(order_history_store, &it);
        char total_str[32];
        snprintf(total_str, sizeof(total_str), "Rs. %d", final);
        gtk_list_store_set(order_history_store, &it,
            0, order_id, 1, "Just Now", 2, "Preparing",
            3, current_payment, 4, total_str, -1);
    }

    // Save receipt BEFORE clearing cart
    generate_receipt(order_id, subtotal, disc_amt, final, current_payment);

    // Confirmation dialog
    GtkWidget *info = gtk_message_dialog_new(GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Order Confirmed!");
    char sec[512];
    snprintf(sec, sizeof(sec),
        "Order ID    : %s\n"
        "Order Type  : %s\n"
        "Payment     : %s\n"
        "Subtotal    : Rs. %d\n"
        "Discount    : Rs. %d\n"
        "Total Paid  : Rs. %d\n\n"
        "Receipt saved to: receipt_%s.txt",
        order_id, current_order_type, current_payment,
        subtotal, disc_amt, final, order_id+1);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(info), "%s", sec);
    gtk_dialog_run(GTK_DIALOG(info));
    gtk_widget_destroy(info);

    // Clear cart
    memset(cart_items, 0, sizeof(cart_items));
    cart_item_count  = 0;
    total_bill       = 0;
    discount_percent = 0;
    discount_amount  = 0;
    gtk_entry_set_text(GTK_ENTRY(coupon_entry), "");
    update_total_display();
    refresh_cart_view();

    // Update analytics if visible
    if (analytics_orders_lbl) {
        int ord = get_total_orders(), rev = get_total_revenue();
        int avg = ord > 0 ? rev / ord : 0;
        char b[64];
        snprintf(b, sizeof(b), "%d",       ord); gtk_label_set_text(GTK_LABEL(analytics_orders_lbl),  b);
        snprintf(b, sizeof(b), "Rs. %d",   rev); gtk_label_set_text(GTK_LABEL(analytics_revenue_lbl), b);
        snprintf(b, sizeof(b), "Rs. %d",   avg); gtk_label_set_text(GTK_LABEL(analytics_avg_lbl),     b);
    }

    show_order_tracker(order_id);
    show_feedback_dialog(order_id);
}

// ==================== SEARCH / CATEGORY ====================
void on_search_changed(GtkEditable *e, gpointer d) {
    (void)d;
    refresh_menu(gtk_entry_get_text(GTK_ENTRY(e)));
}

void on_category_click(GtkWidget *btn, gpointer data) {
    (void)btn;
    current_cat_idx = GPOINTER_TO_INT(data);
    const char *query = search_entry ? gtk_entry_get_text(GTK_ENTRY(search_entry)) : "";
    refresh_menu(query);
}

// ==================== FAVORITES BUTTON ====================
typedef struct { int inv_idx; GtkWidget *fav_lbl; } FavBtnData;

void on_toggle_fav(GtkWidget *btn, gpointer data) {
    (void)btn;
    FavBtnData *fd = (FavBtnData *)data;
    toggle_favorite(inventory[fd->inv_idx].name);
    if (is_favorite(inventory[fd->inv_idx].name)) {
        gtk_button_set_label(GTK_BUTTON(btn), "Saved");
        show_notification("Added to Favorites!", "#C0175F");
    } else {
        gtk_button_set_label(GTK_BUTTON(btn), "Favorite");
        show_notification("Removed from Favorites", "#888888");
    }
    // NOTE: fd is NOT freed here because it lives as long as the button.
    // It will be freed when the button is destroyed via g_object_set_data + destroy notifier below.
}

// ==================== MENU REFRESH ====================
void refresh_menu(const char *query) {
    // Remove all children
    GList *ch = gtk_container_get_children(GTK_CONTAINER(menu_grid));
    for (GList *it = ch; it; it = g_list_next(it))
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(ch);

    const char *cat = CAT_NAMES[current_cat_idx]; // safe: static array
    gboolean is_fav_cat = (strcmp(cat, "Favorites") == 0);

    // Lowercase query for case-insensitive search
    gchar *ql = (query && strlen(query) > 0) ? g_utf8_strdown(query, -1) : NULL;

    int col = 0, row_idx = 0;
    // Responsive: how many columns per row
    int ncols = (UI_SCALE >= 1.7) ? 5 : (UI_SCALE >= 1.3) ? 4 : 3;

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        // Category filter
        gboolean cat_ok = strcmp(cat, "All Items") == 0
                       || strcmp(inventory[i].category, cat) == 0
                       || (is_fav_cat && is_favorite(inventory[i].name));
        if (!cat_ok) continue;

        // Search filter
        if (ql) {
            gchar *nl = g_utf8_strdown(inventory[i].name, -1);
            gboolean match = (g_strstr_len(nl, -1, ql) != NULL);
            g_free(nl);
            if (!match) continue;
        }

        // Build card
        GtkWidget *card = gtk_button_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "product-card");
        gtk_widget_set_size_request(card, SC(200), SC(260));

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(8));
        gtk_container_set_border_width(GTK_CONTAINER(vbox), SC(4));

        // Image
        GtkWidget *img = create_product_image(inventory[i].img_path);
        gtk_widget_set_halign(img, GTK_ALIGN_CENTER);

        // Name label
        GtkWidget *name_lbl = gtk_label_new(inventory[i].name);
        gtk_style_context_add_class(gtk_widget_get_style_context(name_lbl), "item-name");
        gtk_label_set_line_wrap(GTK_LABEL(name_lbl), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(name_lbl), 18);
        gtk_label_set_justify(GTK_LABEL(name_lbl), GTK_JUSTIFY_CENTER);
        gtk_widget_set_halign(name_lbl, GTK_ALIGN_CENTER);

        // Price label
        char p[32];
        snprintf(p, sizeof(p), "Rs. %d", inventory[i].price);
        GtkWidget *price_lbl = gtk_label_new(p);
        gtk_style_context_add_class(gtk_widget_get_style_context(price_lbl), "price-tag");
        gtk_widget_set_halign(price_lbl, GTK_ALIGN_CENTER);

        // Favorite button — allocate FavBtnData on heap, free via destroy signal
        FavBtnData *fd = g_new(FavBtnData, 1);
        fd->inv_idx = i;
        GtkWidget *fav_btn = gtk_button_new_with_label(
            is_favorite(inventory[i].name) ? "Saved" : "Favorite");
        fd->fav_lbl = fav_btn;
        gtk_style_context_add_class(gtk_widget_get_style_context(fav_btn), "btn-fav");
        g_signal_connect(fav_btn, "clicked", G_CALLBACK(on_toggle_fav), fd);
        // Free fd when fav_btn is destroyed
        g_object_set_data_full(G_OBJECT(fav_btn), "favdata", fd, g_free);

        gtk_box_pack_start(GTK_BOX(vbox), img,       TRUE,  TRUE,  0);
        gtk_box_pack_start(GTK_BOX(vbox), name_lbl,  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), price_lbl, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), fav_btn,   FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(card), vbox);

        g_signal_connect(card, "clicked", G_CALLBACK(on_add_to_cart), GINT_TO_POINTER(i));
        gtk_grid_attach(GTK_GRID(menu_grid), card, col, row_idx, 1, 1);

        col++;
        if (col >= ncols) { col = 0; row_idx++; }
    }

    if (ql) g_free(ql);

    // If nothing found, show message
    if (col == 0 && row_idx == 0) {
        GtkWidget *el = gtk_label_new("No items found for your search.");
        gtk_widget_set_halign(el, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(el, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(el, SC(40));
        gtk_grid_attach(GTK_GRID(menu_grid), el, 0, 0, 1, 1);
    }

    gtk_widget_show_all(menu_grid);
}

// ==================== NAVIGATION ====================
void on_nav_click(GtkWidget *btn, gpointer page_id) {
    (void)btn;
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), (const char *)page_id);
}

// ==================== PROFILE HANDLERS ====================
void on_upload_photo(GtkWidget *btn, gpointer d) {
    (void)btn; (void)d;
    GtkWidget *fc = gtk_file_chooser_dialog_new("Select Profile Photo",
        GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open",   GTK_RESPONSE_ACCEPT,
        NULL);
    GtkFileFilter *ff = gtk_file_filter_new();
    gtk_file_filter_set_name(ff, "Image Files");
    gtk_file_filter_add_mime_type(ff, "image/png");
    gtk_file_filter_add_mime_type(ff, "image/jpeg");
    gtk_file_filter_add_mime_type(ff, "image/webp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc), ff);

    if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT) {
        gchar *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(fn, SC(120), SC(120), TRUE, NULL);
        if (pb && profile_pixbuf_widget)
            gtk_image_set_from_pixbuf(GTK_IMAGE(profile_pixbuf_widget), pb);
        if (pb) g_object_unref(pb);
        g_free(fn);
        show_notification("Profile photo updated!", "#1E8A44");
    }
    gtk_widget_destroy(fc);
}

void on_profile_save(GtkWidget *btn, gpointer d) {
    (void)btn; (void)d;
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Profile Saved!");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg),
        "Your profile settings have been updated successfully.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    show_notification("Profile saved!", "#1E8A44");
}

void on_dark_mode(GtkSwitch *sw, gboolean state, gpointer win) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(win));
    if (state) {
        gtk_style_context_add_class(ctx, "dark");
        show_notification("Dark mode ON", "#222222");
    } else {
        gtk_style_context_remove_class(ctx, "dark");
        show_notification("Light mode ON", "#F39C12");
    }
}

void on_reset_coupons(GtkWidget *btn, gpointer d) {
    (void)btn; (void)d;
    coupon_reset_all();
    show_notification("All coupons reset! (PANDA10, SAVE20, FLAT50)", "#1E8A44");
}

// ==================== LOGIN ====================
static GtkWidget *login_user_entry = NULL;
static GtkWidget *login_pass_entry = NULL;

void do_login(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    const char *user = gtk_entry_get_text(GTK_ENTRY(login_user_entry));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(login_pass_entry));
    if (!user || strlen(user)==0 || !pass || strlen(pass)==0) {
        GtkWidget *e = gtk_message_dialog_new(GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Please enter username and password.");
        gtk_dialog_run(GTK_DIALOG(e));
        gtk_widget_destroy(e);
        return;
    }
    if (verify_login(user, pass)) {
        strncpy(logged_in_user, user, 63);
        logged_in_user[63] = '\0';
        gtk_stack_set_visible_child_name(GTK_STACK(login_stack), "app");
        char msg[64];
        snprintf(msg, sizeof(msg), "Welcome, %s!", logged_in_user);
        show_notification(msg, "#C0175F");
    } else {
        GtkWidget *e = gtk_message_dialog_new(GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Login Failed");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(e),
            "Incorrect username or password.\nDefault: admin / 1234");
        gtk_dialog_run(GTK_DIALOG(e));
        gtk_widget_destroy(e);
    }
}

GtkWidget *create_login_page() {
    // Full-page centered box
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(inner, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(inner, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(inner, TRUE);
    gtk_widget_set_hexpand(inner, TRUE);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(14));
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "login-card");
    gtk_widget_set_size_request(card, SC(420), -1);
    gtk_container_set_border_width(GTK_CONTAINER(card), SC(35));

    GtkWidget *title = gtk_label_new("Burhan Panda");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "login-title");
    GtkWidget *sub = gtk_label_new("Food Management System v3.0");
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "login-sub");

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    GtkWidget *ul = gtk_label_new("Username:");
    gtk_widget_set_halign(ul, GTK_ALIGN_START);
    GtkWidget *ub = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(ub), "login-entry");
    login_user_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(login_user_entry), "Enter username");
    gtk_widget_set_hexpand(login_user_entry, TRUE);
    gtk_box_pack_start(GTK_BOX(ub), login_user_entry, TRUE, TRUE, 0);

    GtkWidget *pl = gtk_label_new("Password:");
    gtk_widget_set_halign(pl, GTK_ALIGN_START);
    GtkWidget *pb2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(pb2), "login-entry");
    login_pass_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(login_pass_entry), "Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(login_pass_entry), FALSE);
    gtk_widget_set_hexpand(login_pass_entry, TRUE);
    gtk_box_pack_start(GTK_BOX(pb2), login_pass_entry, TRUE, TRUE, 0);

    // Press Enter on password = login
    g_signal_connect(login_pass_entry, "activate", G_CALLBACK(do_login), NULL);

    GtkWidget *lbtn = gtk_button_new_with_label("Login");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbtn), "login-btn");
    g_signal_connect(lbtn, "clicked", G_CALLBACK(do_login), NULL);

    GtkWidget *hint = gtk_label_new("Default login: admin / 1234");
    gtk_style_context_add_class(gtk_widget_get_style_context(hint), "login-hint");

    gtk_box_pack_start(GTK_BOX(card), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), sub,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), sep,   FALSE, FALSE, SC(8));
    gtk_box_pack_start(GTK_BOX(card), ul,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), ub,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), pl,    FALSE, FALSE, SC(6));
    gtk_box_pack_start(GTK_BOX(card), pb2,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), lbtn,  FALSE, FALSE, SC(10));
    gtk_box_pack_start(GTK_BOX(card), hint,  FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(inner), card,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(outer), inner, TRUE,  TRUE,  0);
    return outer;
}

// ==================== PAGE: DASHBOARD ====================
GtkWidget *create_dashboard() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(14));
    gtk_container_set_border_width(GTK_CONTAINER(vbox), SC(16));
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    // Category bar (scrollable horizontally)
    GtkWidget *cat_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cat_scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_size_request(cat_scroll, -1, SC(48));

    GtkWidget *cat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(6));
    gtk_container_set_border_width(GTK_CONTAINER(cat_box), SC(4));

    int n_cats = (int)(sizeof(CAT_NAMES) / sizeof(CAT_NAMES[0]));
    for (int i = 0; i < n_cats; i++) {
        GtkWidget *btn = gtk_button_new_with_label(CAT_NAMES[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "cat-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_category_click), GINT_TO_POINTER(i));
        gtk_box_pack_start(GTK_BOX(cat_box), btn, FALSE, FALSE, 0);
    }
    gtk_container_add(GTK_CONTAINER(cat_scroll), cat_box);
    gtk_box_pack_start(GTK_BOX(vbox), cat_scroll, FALSE, FALSE, 0);

    // Menu grid (responsive columns)
    menu_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(menu_grid), SC(16));
    gtk_grid_set_column_spacing(GTK_GRID(menu_grid), SC(16));
    refresh_menu("");
    gtk_box_pack_start(GTK_BOX(vbox), menu_grid, TRUE, TRUE, 0);
    return scroll;
}

// ==================== PAGE: ORDERS ====================
GtkWidget *create_orders_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(14));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(24));

    GtkWidget *ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ttl),
        "<span weight='bold' size='x-large'>Order History</span>");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    // 5 columns: ID, Date, Status, Payment, Total
    order_history_store = gtk_list_store_new(5,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(order_history_store));
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree), GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    g_object_set(r, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    const char *hdrs[] = {"Order ID","Date","Status","Payment","Total"};
    int col_widths[]   = {SC(90), SC(120), SC(100), SC(100), SC(90)};
    for (int i = 0; i < 5; i++) {
        GtkTreeViewColumn *c = gtk_tree_view_column_new_with_attributes(
            hdrs[i], r, "text", i, NULL);
        gtk_tree_view_column_set_min_width(c, col_widths[i]);
        gtk_tree_view_column_set_resizable(c, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), c);
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), tree);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *track_btn = gtk_button_new_with_label("Track Latest Order");
    gtk_style_context_add_class(gtk_widget_get_style_context(track_btn), "btn-success");
    g_signal_connect_swapped(track_btn, "clicked",
        G_CALLBACK(show_order_tracker), (gpointer)"#LATEST");
    gtk_box_pack_start(GTK_BOX(btn_row), track_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), ttl,      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), scroll,   TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(box), btn_row,  FALSE, FALSE, 0);
    return box;
}

// ==================== PAGE: ANALYTICS ====================
static void refresh_analytics_stats() {
    if (!analytics_orders_lbl) return;
    int ord = get_total_orders(), rev = get_total_revenue();
    int avg = ord > 0 ? rev / ord : 0;
    char b[64];
    snprintf(b, sizeof(b), "%d",      ord); gtk_label_set_text(GTK_LABEL(analytics_orders_lbl),  b);
    snprintf(b, sizeof(b), "Rs. %d",  rev); gtk_label_set_text(GTK_LABEL(analytics_revenue_lbl), b);
    snprintf(b, sizeof(b), "Rs. %d",  avg); gtk_label_set_text(GTK_LABEL(analytics_avg_lbl),     b);
}

GtkWidget *create_analytics_page() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(18));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(24));
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ttl),
        "<span weight='bold' size='x-large'>Analytics Dashboard</span>");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    // Stat cards in a flow box
    GtkWidget *cards_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(16));

    struct { const char *icon; const char *lbl; GtkWidget **ptr; const char *init; } cards[] = {
        {"Orders",   "Total Orders",    &analytics_orders_lbl,  "0"},
        {"Revenue",  "Total Revenue",   &analytics_revenue_lbl, "Rs. 0"},
        {"Average",  "Avg Order Value", &analytics_avg_lbl,     "Rs. 0"},
        {"Menu",     "Menu Items",      NULL,                   "20"},
    };

    for (int i = 0; i < 4; i++) {
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(8));
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "stat-card");
        gtk_widget_set_size_request(card, SC(160), SC(110));
        gtk_container_set_border_width(GTK_CONTAINER(card), SC(14));

        GtkWidget *icon = gtk_label_new(cards[i].icon);
        GtkWidget *num  = gtk_label_new(cards[i].init);
        gtk_style_context_add_class(gtk_widget_get_style_context(num),  "stat-number");
        GtkWidget *lbl  = gtk_label_new(cards[i].lbl);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl),  "stat-label");

        if (cards[i].ptr) *(cards[i].ptr) = num;

        gtk_widget_set_halign(icon, GTK_ALIGN_START);
        gtk_widget_set_halign(num,  GTK_ALIGN_START);
        gtk_widget_set_halign(lbl,  GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(card), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), num,  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), lbl,  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(cards_row), card, FALSE, FALSE, 0);
    }

    // Category distribution
    GtkWidget *dist_ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(dist_ttl), "<b>Category Distribution</b>");
    gtk_widget_set_halign(dist_ttl, GTK_ALIGN_START);

    GtkWidget *dist_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(8));
    gtk_style_context_add_class(gtk_widget_get_style_context(dist_box), "stat-card");
    gtk_container_set_border_width(GTK_CONTAINER(dist_box), SC(16));

    const char *dcat[]  = {"Burgers","Pizza","Pasta","BBQ","Rice","Drinks","Desserts"};
    double dvals[]      = {0.20, 0.18, 0.15, 0.12, 0.12, 0.13, 0.10};
    for (int i = 0; i < 7; i++) {
        GtkWidget *row  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
        GtkWidget *cl   = gtk_label_new(dcat[i]);
        gtk_widget_set_size_request(cl, SC(80), -1);
        gtk_widget_set_halign(cl, GTK_ALIGN_START);
        GtkWidget *bar  = gtk_progress_bar_new();
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar), dvals[i]);
        char ps[16]; snprintf(ps, sizeof(ps), "%.0f%%", dvals[i]*100);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(bar), ps);
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(bar), TRUE);
        gtk_widget_set_hexpand(bar, TRUE);
        gtk_box_pack_start(GTK_BOX(row), cl, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), bar, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(dist_box), row, FALSE, FALSE, 0);
    }

    GtkWidget *ref_btn = gtk_button_new_with_label("Refresh Stats");
    gtk_style_context_add_class(gtk_widget_get_style_context(ref_btn), "btn-success");
    g_signal_connect_swapped(ref_btn, "clicked",
        G_CALLBACK(refresh_analytics_stats), NULL);

    gtk_box_pack_start(GTK_BOX(box), ttl,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), cards_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), dist_ttl,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), dist_box,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), ref_btn,   FALSE, FALSE, 0);

    refresh_analytics_stats();
    return scroll;
}

// ==================== PAGE: PROFILE ====================
GtkWidget *create_profile_page() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(12));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(30));
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ttl),
        "<span weight='bold' size='x-large'>Profile Settings</span>");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    profile_pixbuf_widget = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_halign(profile_pixbuf_widget, GTK_ALIGN_START);

    GtkWidget *ph_btn = gtk_button_new_with_label("Upload Photo");
    gtk_style_context_add_class(gtk_widget_get_style_context(ph_btn), "cat-btn");
    g_signal_connect(ph_btn, "clicked", G_CALLBACK(on_upload_photo), NULL);

    // Form fields
    struct { const char *lbl; const char *ph; gboolean pw; } fields[] = {
        {"Full Name:",       "Enter your full name",         FALSE},
        {"Email:",           "Enter email address",          FALSE},
        {"Phone:",           "+92 300 0000000",              FALSE},
        {"Address:",         "Delivery address",             FALSE},
        {"Current Password:","Current password",             TRUE},
        {"New Password:",    "New password",                 TRUE},
    };
    GtkWidget *entries[6];
    GtkWidget *fg = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(fg), SC(8));
    gtk_grid_set_column_spacing(GTK_GRID(fg), SC(12));

    for (int i = 0; i < 6; i++) {
        GtkWidget *l = gtk_label_new(fields[i].lbl);
        gtk_widget_set_halign(l, GTK_ALIGN_END);
        entries[i] = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entries[i]), fields[i].ph);
        if (fields[i].pw) gtk_entry_set_visibility(GTK_ENTRY(entries[i]), FALSE);
        gtk_widget_set_hexpand(entries[i], TRUE);
        gtk_grid_attach(GTK_GRID(fg), l,          0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(fg), entries[i], 1, i, 1, 1);
    }
    // Pre-fill name
    gtk_entry_set_text(GTK_ENTRY(entries[0]), "Burhan Ahmed");

    GtkWidget *save = gtk_button_new_with_label("Save Profile");
    gtk_style_context_add_class(gtk_widget_get_style_context(save), "btn-checkout");
    g_signal_connect(save, "clicked", G_CALLBACK(on_profile_save), NULL);

    gtk_box_pack_start(GTK_BOX(box), ttl,                  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), profile_pixbuf_widget, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), ph_btn,               FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), fg,                   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), save,                 FALSE, FALSE, SC(10));
    return scroll;
}

// ==================== PAGE: SETTINGS ====================
GtkWidget *create_settings_page(GtkWidget *win) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(16));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(30));
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *ttl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ttl),
        "<span weight='bold' size='x-large'>App Settings</span>");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);

    // Dark mode toggle
    GtkWidget *dm_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *dm_lbl = gtk_label_new("Dark Mode (experimental)");
    gtk_widget_set_hexpand(dm_lbl, TRUE);
    gtk_widget_set_halign(dm_lbl, GTK_ALIGN_START);
    GtkWidget *dm_sw  = gtk_switch_new();
    g_signal_connect(dm_sw, "state-set", G_CALLBACK(on_dark_mode), win);
    gtk_box_pack_start(GTK_BOX(dm_row), dm_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(dm_row), dm_sw,  FALSE, FALSE, 0);

    // Reset coupons
    GtkWidget *rc_row  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *rc_lbl  = gtk_label_new("Reset All Coupons (for testing)");
    gtk_widget_set_hexpand(rc_lbl, TRUE);
    gtk_widget_set_halign(rc_lbl, GTK_ALIGN_START);
    GtkWidget *rc_btn  = gtk_button_new_with_label("Reset Coupons");
    gtk_style_context_add_class(gtk_widget_get_style_context(rc_btn), "btn-success");
    g_signal_connect(rc_btn, "clicked", G_CALLBACK(on_reset_coupons), NULL);
    gtk_box_pack_start(GTK_BOX(rc_row), rc_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rc_row), rc_btn, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    // Coupons hint
    GtkWidget *coupon_info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(coupon_info),
        "<b>Available Coupons:</b>\n"
        "  PANDA10  =  10% off  (min order Rs. 500)\n"
        "  SAVE20   =  20% off  (min order Rs. 1000)\n"
        "  FLAT50   =  50% off  (min order Rs. 2000)");
    gtk_widget_set_halign(coupon_info, GTK_ALIGN_START);

    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    // About
    GtkWidget *about = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(about),
        "<b>About Burhan Panda v3.0</b>\n"
        "<small>Built with GTK3 + SQLite3 (C Language)\n"
        "Features: Login, Responsive UI, Cart with Qty,\n"
        "Coupons, Receipts, Analytics, Favorites, Tracker\n"
        "Works on Laptop and Projector screens</small>");
    gtk_widget_set_halign(about, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(box), ttl,         FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), dm_row,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), rc_row,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sep,          FALSE, FALSE, SC(6));
    gtk_box_pack_start(GTK_BOX(box), coupon_info,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sep2,         FALSE, FALSE, SC(6));
    gtk_box_pack_start(GTK_BOX(box), about,        FALSE, FALSE, 0);
    return scroll;
}

// ==================== MAIN APP UI BUILD ====================
GtkWidget *build_app_ui() {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // ---- HEADER ----
    GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(12));
    gtk_style_context_add_class(gtk_widget_get_style_context(hdr), "top-header");

    GtkWidget *logo = gtk_label_new("Burhan Panda");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "logo-text");

    GtkWidget *hdr_spacer = gtk_label_new("");
    gtk_widget_set_hexpand(hdr_spacer, TRUE);

    // Search inside its own box so CSS targets ".search-bar entry"
    GtkWidget *srch_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(srch_box), "search-bar");
    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search food...");
    gtk_widget_set_size_request(search_entry, SC(300), -1);
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(srch_box), search_entry, TRUE, TRUE, 0);

    GtkWidget *usr_lbl = gtk_label_new(logged_in_user);
    gtk_style_context_add_class(gtk_widget_get_style_context(usr_lbl), "user-badge");

    gtk_box_pack_start(GTK_BOX(hdr), logo,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hdr), hdr_spacer, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(hdr), srch_box,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hdr), usr_lbl,    FALSE, FALSE, SC(8));
    gtk_box_pack_start(GTK_BOX(root), hdr, FALSE, FALSE, 0);

    // ---- NOTIFICATION REVEALER ----
    notif_revealer  = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(notif_revealer),
        GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(GTK_REVEALER(notif_revealer), 250);

    notif_box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(notif_box_widget), "notif-box");
    notif_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(notif_label), "notif-text");
    gtk_widget_set_halign(notif_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(notif_label, TRUE);
    gtk_box_pack_start(GTK_BOX(notif_box_widget), notif_label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(notif_revealer), notif_box_widget);
    gtk_box_pack_start(GTK_BOX(root), notif_revealer, FALSE, FALSE, 0);

    // ---- BODY (sidebar + stack + cart) ----
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(root), body, TRUE, TRUE, 0);

    // Sidebar
    GtkWidget *side = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(4));
    gtk_style_context_add_class(gtk_widget_get_style_context(side), "sidebar");
    gtk_widget_set_size_request(side, SC(170), -1);

    const char *nav_labels[] = {"Dashboard", "Orders", "Analytics", "Profile", "Settings"};
    const char *nav_pages[]  = {"dash", "orders", "analytics", "prof", "set"};
    for (int i = 0; i < 5; i++) {
        GtkWidget *btn = gtk_button_new_with_label(nav_labels[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "nav-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_click), (gpointer)nav_pages[i]);
        gtk_box_pack_start(GTK_BOX(side), btn, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(body), side, FALSE, FALSE, 0);

    // Page stack
    main_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(main_stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(main_stack), 180);
    gtk_stack_add_named(GTK_STACK(main_stack), create_dashboard(),      "dash");
    gtk_stack_add_named(GTK_STACK(main_stack), create_orders_page(),    "orders");
    gtk_stack_add_named(GTK_STACK(main_stack), create_analytics_page(), "analytics");
    gtk_stack_add_named(GTK_STACK(main_stack), create_profile_page(),   "prof");
    gtk_stack_add_named(GTK_STACK(main_stack), create_settings_page(main_window), "set");
    gtk_box_pack_start(GTK_BOX(body), main_stack, TRUE, TRUE, 0);

    // ---- CART PANEL ----
    GtkWidget *cart = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(8));
    gtk_style_context_add_class(gtk_widget_get_style_context(cart), "cart-panel");
    gtk_widget_set_size_request(cart, SC(300), -1);
    gtk_container_set_border_width(GTK_CONTAINER(cart), SC(14));

    GtkWidget *cart_ttl = gtk_label_new("Your Basket");
    gtk_style_context_add_class(gtk_widget_get_style_context(cart_ttl), "cart-title");
    gtk_widget_set_halign(cart_ttl, GTK_ALIGN_START);

    // Order type row
    GtkWidget *ot_lbl = gtk_label_new("Order Type:");
    gtk_widget_set_halign(ot_lbl, GTK_ALIGN_START);
    GtkWidget *ot_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(8));
    GtkWidget *dine_btn = gtk_button_new_with_label("Dine-In");
    GtkWidget *take_btn = gtk_button_new_with_label("Takeaway");
    gtk_style_context_add_class(gtk_widget_get_style_context(dine_btn), "cat-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(take_btn), "cat-btn");
    g_signal_connect(dine_btn, "clicked", G_CALLBACK(on_select_dine), NULL);
    g_signal_connect(take_btn, "clicked", G_CALLBACK(on_select_take), NULL);
    gtk_box_pack_start(GTK_BOX(ot_row), dine_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ot_row), take_btn, TRUE, TRUE, 0);

    // Cart TreeView: Name | Qty | Price
    cart_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    cart_view  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cart_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(cart_view), TRUE);
    g_signal_connect(cart_view, "row-activated", G_CALLBACK(on_remove_item), NULL);

    GtkCellRenderer *cr = gtk_cell_renderer_text_new();
    g_object_set(cr, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    GtkTreeViewColumn *c0 = gtk_tree_view_column_new_with_attributes("Item",  cr, "text", 0, NULL);
    GtkTreeViewColumn *c1 = gtk_tree_view_column_new_with_attributes("Qty",   cr, "text", 1, NULL);
    GtkTreeViewColumn *c2 = gtk_tree_view_column_new_with_attributes("Price", cr, "text", 2, NULL);
    gtk_tree_view_column_set_expand(c0, TRUE);
    gtk_tree_view_column_set_min_width(c1, SC(40));
    gtk_tree_view_column_set_min_width(c2, SC(70));
    gtk_tree_view_append_column(GTK_TREE_VIEW(cart_view), c0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cart_view), c1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cart_view), c2);

    GtkWidget *cart_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(cart_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(cart_scroll), cart_view);

    GtkWidget *remove_hint = gtk_label_new("(double-click item to remove)");
    gtk_widget_set_halign(remove_hint, GTK_ALIGN_CENTER);

    // Coupon row
    GtkWidget *cp_lbl = gtk_label_new("Coupon Code:");
    gtk_widget_set_halign(cp_lbl, GTK_ALIGN_START);
    GtkWidget *cp_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(4));
    coupon_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(coupon_entry), "e.g. PANDA10");
    gtk_widget_set_hexpand(coupon_entry, TRUE);
    GtkWidget *ap_btn = gtk_button_new_with_label("Apply");
    gtk_style_context_add_class(gtk_widget_get_style_context(ap_btn), "btn-success");
    GtkWidget *rm_btn = gtk_button_new_with_label("X");
    gtk_style_context_add_class(gtk_widget_get_style_context(rm_btn), "btn-danger");
    g_signal_connect(ap_btn, "clicked", G_CALLBACK(on_apply_coupon), NULL);
    g_signal_connect(rm_btn, "clicked", G_CALLBACK(on_remove_coupon), NULL);
    gtk_box_pack_start(GTK_BOX(cp_row), coupon_entry, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(cp_row), ap_btn,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cp_row), rm_btn,       FALSE, FALSE, 0);

    // Discount label (hidden until coupon applied)
    discount_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(discount_label), "discount-lbl");
    gtk_widget_set_visible(discount_label, FALSE);
    gtk_widget_set_halign(discount_label, GTK_ALIGN_START);

    // Total
    total_label = gtk_label_new("Rs. 0");
    gtk_style_context_add_class(gtk_widget_get_style_context(total_label), "total-lbl");
    gtk_widget_set_halign(total_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(total_label, SC(6));

    // Checkout
    GtkWidget *chk = gtk_button_new_with_label("Checkout");
    gtk_style_context_add_class(gtk_widget_get_style_context(chk), "btn-checkout");
    g_signal_connect(chk, "clicked", G_CALLBACK(on_checkout), NULL);

    gtk_box_pack_start(GTK_BOX(cart), cart_ttl,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), ot_lbl,      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), ot_row,      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), cart_scroll, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(cart), remove_hint, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), cp_lbl,      FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(cart), cp_row,      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), discount_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), total_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), chk,         FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(body), cart, FALSE, FALSE, 0);
    return root;
}

// ==================== MAIN ====================
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    gtk_init(&argc, &argv);

    // Compute UI scale BEFORE loading CSS
    compute_ui_scale();

    init_database();
    load_css();

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window),
        "Burhan Panda - Food Management v3.0");
    // Start maximized so it fills laptop / projector
    gtk_window_maximize(GTK_WINDOW(main_window));
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

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
