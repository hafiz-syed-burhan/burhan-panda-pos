// ============================================================
//   BURHAN PANDA FOOD MANAGEMENT SYSTEM  v2.0 - 
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
    const char *sql1 = "CREATE TABLE IF NOT EXISTS orders("
                       "id TEXT, date TEXT, status TEXT, total INT, "
                       "payment_method TEXT, order_type TEXT, feedback INT);";
    sqlite3_exec(db, sql1, 0, 0, &err_msg);

    const char *sql2 = "CREATE TABLE IF NOT EXISTS favorites("
                       "item_name TEXT UNIQUE);";
    sqlite3_exec(db, sql2, 0, 0, &err_msg);

    const char *sql3 = "CREATE TABLE IF NOT EXISTS users("
                       "username TEXT UNIQUE, password TEXT, fullname TEXT, email TEXT);";
    sqlite3_exec(db, sql3, 0, 0, &err_msg);

    const char *sql4 = "INSERT OR IGNORE INTO users VALUES('admin','1234','Burhan Ahmed','admin@panda.pk');";
    sqlite3_exec(db, sql4, 0, 0, &err_msg);

    const char *sql5 = "CREATE TABLE IF NOT EXISTS coupons("
                       "code TEXT UNIQUE, discount_percent INT, min_order INT, used INT DEFAULT 0);";
    sqlite3_exec(db, sql5, 0, 0, &err_msg);

    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('PANDA10',10,500,0);", 0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('SAVE20',20,1000,0);",  0, 0, 0);
    sqlite3_exec(db, "INSERT OR IGNORE INTO coupons VALUES('FLAT50',50,2000,0);",  0, 0, 0);

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

void coupon_reset_all() {
    sqlite3_exec(db, "UPDATE coupons SET used=0;", 0, 0, 0);
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

// ==================== RESPONSIVE SCALE ====================
// Ye function screen resolution dekh ke UI scale set karta hai
// Laptop (1366px) = 1.0, FHD (1920px) = 1.25, Projector (varies) = auto
static double UI_SCALE = 1.0;

void compute_ui_scale() {
    GdkScreen *screen = gdk_screen_get_default();
    int sw = gdk_screen_get_width(screen);
    printf("[INFO] Screen width: %dpx\n", sw);
    if      (sw >= 3840) UI_SCALE = 2.0;
    else if (sw >= 2560) UI_SCALE = 1.6;
    else if (sw >= 1920) UI_SCALE = 1.25;
    else if (sw >= 1600) UI_SCALE = 1.1;
    else if (sw >= 1366) UI_SCALE = 1.0;
    else                 UI_SCALE = 0.88;
    printf("[INFO] UI_SCALE = %.2f\n", UI_SCALE);
}

// Scale kisi bhi integer value ko screen k hisaab se
static inline int SC(int v) { return (int)(v * UI_SCALE); }

// ==================== DARK MODE STATE ====================
static gboolean dark_mode_on = FALSE;

// ==================== CSS - PROPERLY FIXED ====================
// GTK3 CSS rules:
//   - font-size: Npt  (pt units, not px)
//   - font-weight: bold / normal (not numbers)
//   - label color: "label.classname { color: ... }" style
//   - button label color alag se set karni padti hai
//   - box-shadow / letter-spacing GTK3 mein kaam nahi karta

static GtkCssProvider *css_provider = NULL;

void load_css() {
    // Colors change based on dark mode
    const char *BG       = dark_mode_on ? "#121212" : "#F4F4F4";
    const char *SURFACE  = dark_mode_on ? "#1E1E1E" : "#FFFFFF";
    const char *SURFACE2 = dark_mode_on ? "#2A2A2A" : "#F8F8F8";
    const char *BORDER   = dark_mode_on ? "#333333" : "#EEEEEE";
    const char *TEXT     = dark_mode_on ? "#F0F0F0" : "#222222";
    const char *TEXT2    = dark_mode_on ? "#AAAAAA" : "#666666";
    const char *PINK     = "#E21B70";
    const char *PINK_DRK = "#B01055";
    const char *GREEN    = "#27AE60";
    const char *RED      = "#E74C3C";
    const char *HDR_BG   = dark_mode_on ? "#1A0A12" : "#E21B70";

    // Font sizes in pt — scaled per screen resolution
    // At UI_SCALE=1.0 (1366px laptop): normal=10pt, large=12pt, xlarge=15pt
    int pt_small  = (int)(8  * UI_SCALE);
    int pt_normal = (int)(10 * UI_SCALE);
    int pt_medium = (int)(11 * UI_SCALE);
    int pt_large  = (int)(13 * UI_SCALE);
    int pt_xlarge = (int)(15 * UI_SCALE);
    int pt_huge   = (int)(18 * UI_SCALE);

    char css[12000];
    int  len = 0;
    #define A(...) len += snprintf(css+len, sizeof(css)-len, __VA_ARGS__)

    // === GLOBAL ===
    A("* { font-family: 'Ubuntu', 'DejaVu Sans', 'Noto Sans', sans-serif; }\n");
    A("window { background-color: %s; }\n", BG);

    // === HEADER ===
    A(".top-header { background-color: %s; padding: %dpx %dpx; }\n",
      HDR_BG, SC(11), SC(28));

    // === LOGO ===
    // GTK3: label ka color 'label.classname' se set hota hai
    A("label.logo-text { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_xlarge);

    // === SEARCH BAR ===
    A("entry.search-entry {\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx %dpx;\n"
      "  font-size: %dpt;\n"
      "  background-color: %s;\n"
      "  color: %s;\n"
      "  border: 1px solid %s;\n"
      "}\n",
      SC(20), SC(6), SC(14), pt_normal,
      dark_mode_on ? "#2A2A3A" : "#FFFFFF",
      TEXT, dark_mode_on ? "#444444" : "#DDDDDD");

    // === USER BADGE ===
    A("label.user-badge { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_normal);

    // === SIDEBAR ===
    A(".sidebar {\n"
      "  background-color: %s;\n"
      "  border-right: 1px solid %s;\n"
      "  padding: %dpx %dpx;\n"
      "}\n", SURFACE, BORDER, SC(12), SC(6));

    // === NAV BUTTONS ===
    // GTK3 mein button ka label alag widget hota hai — dono ko style karo
    A("button.nav-btn {\n"
      "  background-color: transparent;\n"
      "  border: none;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx %dpx;\n"
      "  margin-bottom: %dpx;\n"
      "  outline: none;\n"
      "}\n", SC(10), SC(9), SC(14), SC(3));
    A("button.nav-btn label { color: %s; font-size: %dpt; font-weight: bold; }\n", TEXT, pt_medium);
    A("button.nav-btn:hover { background-color: %s; }\n", PINK);
    A("button.nav-btn:hover label { color: #FFFFFF; }\n");

    // === CATEGORY BUTTONS ===
    A("button.category-btn {\n"
      "  background-color: %s;\n"
      "  border: 1px solid %s;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx %dpx;\n"
      "  outline: none;\n"
      "}\n", SURFACE2, BORDER, SC(16), SC(5), SC(12));
    A("button.category-btn label { color: %s; font-size: %dpt; font-weight: bold; }\n", TEXT, pt_normal);
    A("button.category-btn:hover { background-color: %s; }\n", PINK);
    A("button.category-btn:hover label { color: #FFFFFF; }\n");

    // === PRODUCT CARDS ===
    A("button.product-card {\n"
      "  background-color: %s;\n"
      "  border: 1px solid %s;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx;\n"
      "  outline: none;\n"
      "}\n", SURFACE, BORDER, SC(16), SC(10));
    A("button.product-card:hover {\n"
      "  background-color: %s;\n"
      "  border: 2px solid %s;\n"
      "}\n", dark_mode_on ? "#2A1020" : "#FFF0F5", PINK);

    // Item name label on card — ye pehle nazar nahi aa raha tha
    A("label.item-name {\n"
      "  color: %s;\n"
      "  font-size: %dpt;\n"
      "  font-weight: bold;\n"
      "}\n", TEXT, pt_normal);

    // Price tag
    A("label.price-tag {\n"
      "  color: %s;\n"
      "  font-size: %dpt;\n"
      "  font-weight: bold;\n"
      "}\n", PINK, pt_medium);

    // === FAVORITE BUTTON ===
    A("button.btn-fav {\n"
      "  background-color: transparent;\n"
      "  border: 1px solid %s;\n"
      "  border-radius: %dpx;\n"
      "  padding: 2px %dpx;\n"
      "  outline: none;\n"
      "}\n", PINK, SC(14), SC(8));
    A("button.btn-fav label { color: %s; font-size: %dpt; }\n", PINK, pt_small);
    A("button.btn-fav.saved { background-color: %s; }\n", PINK);
    A("button.btn-fav.saved label { color: #FFFFFF; }\n");

    // === CART PANEL ===
    A(".cart-panel {\n"
      "  background-color: %s;\n"
      "  border-left: 1px solid %s;\n"
      "  padding: %dpx %dpx;\n"
      "}\n", SURFACE, BORDER, SC(12), SC(10));
    A("label.cart-title { color: %s; font-size: %dpt; font-weight: bold; }\n", TEXT, pt_large);
    A("label.section-lbl { color: %s; font-size: %dpt; }\n", TEXT2, pt_small);
    A("label.total-lbl { color: %s; font-size: %dpt; font-weight: bold; }\n", PINK, (int)(20*UI_SCALE));
    A("label.discount-lbl { color: %s; font-size: %dpt; font-weight: bold; }\n", GREEN, pt_normal);
    A("label.hint-lbl { color: %s; font-size: %dpt; }\n", TEXT2, pt_small);

    // === CHECKOUT BUTTON ===
    A("button.btn-checkout {\n"
      "  background-color: %s;\n"
      "  border: none;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx;\n"
      "  outline: none;\n"
      "}\n", PINK, SC(10), SC(11));
    A("button.btn-checkout label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_large);
    A("button.btn-checkout:hover { background-color: %s; }\n", PINK_DRK);

    // === SUCCESS BUTTON ===
    A("button.btn-success {\n"
      "  background-color: %s;\n"
      "  border: none;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx %dpx;\n"
      "  outline: none;\n"
      "}\n", GREEN, SC(8), SC(5), SC(10));
    A("button.btn-success label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_normal);

    // === DANGER BUTTON ===
    A("button.btn-danger {\n"
      "  background-color: %s;\n"
      "  border: none;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx %dpx;\n"
      "  outline: none;\n"
      "}\n", RED, SC(8), SC(5), SC(10));
    A("button.btn-danger label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_normal);

    // === NOTIFICATION ===
    A(".notification-bar { border-radius: %dpx; padding: %dpx %dpx; }\n",
      SC(8), SC(7), SC(14));
    A("label.notif-text { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_normal);

    // === STAT CARDS ===
    A(".stat-card {\n"
      "  background-color: %s;\n"
      "  border: 1px solid %s;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx;\n"
      "}\n", SURFACE, BORDER, SC(16), SC(14));
    A("label.stat-number { color: %s; font-size: %dpt; font-weight: bold; }\n", PINK, pt_huge);
    A("label.stat-label  { color: %s; font-size: %dpt; }\n", TEXT2, pt_normal);

    // === LOGIN PAGE ===
    A(".login-card {\n"
      "  background-color: %s;\n"
      "  border-radius: %dpx;\n"
      "  border: 1px solid %s;\n"
      "}\n", SURFACE, SC(20), BORDER);
    A("label.login-title { color: %s; font-size: %dpt; font-weight: bold; }\n", PINK, pt_huge);
    A("label.login-sub   { color: %s; font-size: %dpt; }\n", TEXT2, pt_normal);
    A("label.login-hint  { color: %s; font-size: %dpt; }\n", TEXT2, pt_small);
    A("entry.login-entry {\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx %dpx;\n"
      "  font-size: %dpt;\n"
      "  background-color: %s;\n"
      "  color: %s;\n"
      "  border: 2px solid %s;\n"
      "}\n",
      SC(10), SC(9), SC(12), pt_medium,
      dark_mode_on ? "#2A2A3A" : "#F8F8F8",
      TEXT, BORDER);
    A("button.login-btn {\n"
      "  background-color: %s;\n"
      "  border: none;\n"
      "  border-radius: %dpx;\n"
      "  padding: %dpx;\n"
      "  outline: none;\n"
      "}\n", PINK, SC(12), SC(11));
    A("button.login-btn label { color: #FFFFFF; font-size: %dpt; font-weight: bold; }\n", pt_large);
    A("button.login-btn * { color: #FFFFFF; }\n");
    A("button.login-btn:hover { background-color: %s; }\n", PINK_DRK);

    // === PAGE TITLES ===
    A("label.page-title { color: %s; font-size: %dpt; font-weight: bold; }\n", PINK, pt_xlarge);

    // === TREEVIEW ===
    A("treeview { background-color: %s; color: %s; font-size: %dpt; }\n",
      SURFACE, TEXT, pt_normal);
    A("treeview:selected { background-color: %s; color: #FFFFFF; }\n", PINK);
    A("treeview header button { background-color: %s; }\n", SURFACE2);
    A("treeview header button label { color: %s; font-size: %dpt; font-weight: bold; }\n",
      PINK, pt_normal);

    // === ORDER TRACKER STEPS ===
    A("label.progress-step-done {\n"
      "  background-color: %s; color: #FFFFFF;\n"
      "  border-radius: %dpx; padding: 2px %dpx;\n"
      "  font-size: %dpt; font-weight: bold;\n"
      "}\n", GREEN, SC(12), SC(8), pt_small);
    A("label.progress-step-active {\n"
      "  background-color: %s; color: #FFFFFF;\n"
      "  border-radius: %dpx; padding: 2px %dpx;\n"
      "  font-size: %dpt; font-weight: bold;\n"
      "}\n", PINK, SC(12), SC(8), pt_small);
    A("label.progress-step-todo {\n"
      "  background-color: %s; color: %s;\n"
      "  border-radius: %dpx; padding: 2px %dpx;\n"
      "  font-size: %dpt;\n"
      "}\n",
      dark_mode_on ? "#333333" : "#EEEEEE",
      dark_mode_on ? "#888888" : "#888888",
      SC(12), SC(8), pt_small);

    // === PROGRESS BAR ===
    A("progressbar trough {\n"
      "  background-color: %s;\n"
      "  border-radius: %dpx;\n"
      "  min-height: %dpx;\n"
      "}\n", dark_mode_on ? "#333333" : "#E8E8E8", SC(4), SC(12));
    A("progressbar progress {\n"
      "  background-color: %s;\n"
      "  border-radius: %dpx;\n"
      "}\n", PINK, SC(4));

    // === ENTRY (general) ===
    A("entry {\n"
      "  background-color: %s;\n"
      "  color: %s;\n"
      "  border: 1px solid %s;\n"
      "  border-radius: %dpx;\n"
      "}\n",
      dark_mode_on ? "#2A2A3A" : "#FFFFFF",
      TEXT, BORDER, SC(6));

    // === SCROLLBAR ===
    A("scrollbar slider { background-color: %s; border-radius: %dpx; min-width: %dpx; }\n",
      dark_mode_on ? "#555555" : "#CCCCCC", SC(4), SC(6));

    // === SEPARATOR ===
    A("separator { background-color: %s; }\n", BORDER);

    // === DIALOG ===
    A("dialog { background-color: %s; }\n", SURFACE);
    A("dialog label { color: %s; }\n", TEXT);

    #undef A

    // Remove old provider and apply new one
    if (css_provider) {
        gtk_style_context_remove_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider));
        g_object_unref(css_provider);
    }
    css_provider = gtk_css_provider_new();
    GError *error = NULL;
    gtk_css_provider_load_from_data(css_provider, css, -1, &error);
    if (error) {
        fprintf(stderr, "[CSS Error] %s\n", error->message);
        g_error_free(error);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
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

FoodItem inventory[] = {
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

// ==================== GLOBAL WIDGETS & STATE ====================
GtkWidget    *main_window;
GtkWidget    *login_stack;
GtkWidget    *main_stack;
GtkWidget    *menu_flowbox;
GtkWidget    *cart_view;
GtkWidget    *total_label;
GtkWidget    *discount_label;
GtkWidget    *coupon_entry;
GtkWidget    *search_entry;
GtkWidget    *notif_revealer;
GtkWidget    *notif_label;
GtkWidget    *notif_box_inner;   // notification colored box
GtkWidget    *order_type_dine_btn;
GtkWidget    *order_type_take_btn;
GtkWidget    *analytics_orders_lbl;
GtkWidget    *analytics_revenue_lbl;
GtkWidget    *analytics_avg_lbl;
GtkWidget    *profile_pixbuf_widget;
GtkWidget    *order_history_tree;

GtkListStore *cart_store;
GtkListStore *order_history_store;

int   total_bill       = 0;
int   discount_percent = 0;
int   discount_amount  = 0;
char *current_category = "All Items";
char  current_order_type[32] = "Dine-In";
char  current_payment[32]    = "Cash";
char  logged_in_user[64]     = "";
guint notif_timeout_id       = 0;

CartItem cart_items[100];
int      cart_item_count = 0;

// ==================== FORWARD DECLARATIONS ====================
void update_total_display();
void refresh_menu(const char *search_query);
void refresh_cart_view();
void show_notification(const char *message, const char *color);

// ==================== NOTIFICATION SYSTEM ====================
gboolean hide_notification_cb(gpointer data) {
    (void)data;
    gtk_revealer_set_reveal_child(GTK_REVEALER(notif_revealer), FALSE);
    notif_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void show_notification(const char *message, const char *color) {
    if (!notif_label || !notif_revealer) return;
    gtk_label_set_text(GTK_LABEL(notif_label), message);

    // Dynamically recolor notification box
    char css[256];
    snprintf(css, sizeof(css),
        ".notification-bar { background-color: %s; border-radius: %dpx; padding: %dpx %dpx; }",
        color, SC(8), SC(7), SC(14));
    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, css, -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(notif_box_inner),
        GTK_STYLE_PROVIDER(cp),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(cp);

    gtk_revealer_set_reveal_child(GTK_REVEALER(notif_revealer), TRUE);
    if (notif_timeout_id) g_source_remove(notif_timeout_id);
    notif_timeout_id = g_timeout_add(3000, hide_notification_cb, NULL);
}

// ==================== IMAGE HELPER ====================
GtkWidget *create_product_image(const char *path) {
    int w = SC(140), h = SC(100);
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, w, h, TRUE, NULL);
    if (pb) {
        GtkWidget *img = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb);
        gtk_widget_set_halign(img, GTK_ALIGN_CENTER);
        return img;
    }
    // Fallback: placeholder box with fixed size
    GtkWidget *ph = gtk_label_new("[No Image]");
    gtk_widget_set_size_request(ph, w, h);
    return ph;
}

// ==================== TOTAL DISPLAY ====================
void update_total_display() {
    discount_amount = (total_bill * discount_percent) / 100;
    int final_total = total_bill - discount_amount;
    char buf[64];
    sprintf(buf, "Rs. %d", final_total);
    gtk_label_set_text(GTK_LABEL(total_label), buf);

    if (discount_percent > 0) {
        char disc_buf[80];
        sprintf(disc_buf, "%d%% OFF  - Rs.%d saved!", discount_percent, discount_amount);
        gtk_label_set_text(GTK_LABEL(discount_label), disc_buf);
        gtk_widget_set_visible(discount_label, TRUE);
    } else {
        gtk_widget_set_visible(discount_label, FALSE);
    }
}

// ==================== CART QUANTITY MANAGEMENT ====================
void refresh_cart_view() {
    gtk_list_store_clear(cart_store);
    for (int i = 0; i < cart_item_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(cart_store, &iter);
        char qty_str[16];
        sprintf(qty_str, "x%d", cart_items[i].quantity);
        char price_str[32];
        sprintf(price_str, "Rs.%d", cart_items[i].price * cart_items[i].quantity);
        gtk_list_store_set(cart_store, &iter,
                           0, cart_items[i].name,
                           1, qty_str,
                           2, price_str,
                           -1);
    }
}

void on_add_to_cart(GtkWidget *w, gpointer data) {
    (void)w;
    int idx = GPOINTER_TO_INT(data);

    // Check if already in cart — increase quantity
    for (int i = 0; i < cart_item_count; i++) {
        if (strcmp(cart_items[i].name, inventory[idx].name) == 0) {
            cart_items[i].quantity++;
            total_bill += inventory[idx].price;
            update_total_display();
            refresh_cart_view();
            char msg[128];
            snprintf(msg, sizeof(msg), "Added: %s (x%d)", inventory[idx].name, cart_items[i].quantity);
            show_notification(msg, "#27AE60");
            return;
        }
    }
    // New item
    if (cart_item_count < 100) {
        strncpy(cart_items[cart_item_count].name, inventory[idx].name, 127);
        cart_items[cart_item_count].name[127] = '\0';
        cart_items[cart_item_count].price    = inventory[idx].price;
        cart_items[cart_item_count].quantity = 1;
        cart_item_count++;
        total_bill += inventory[idx].price;
        update_total_display();
        refresh_cart_view();
        char msg[128];
        snprintf(msg, sizeof(msg), "Added: %s", inventory[idx].name);
        show_notification(msg, "#27AE60");
    }
}

void on_remove_item(GtkTreeView *tree_view, GtkTreePath *path,
                    GtkTreeViewColumn *column, gpointer user_data) {
    (void)column; (void)user_data;
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
    show_notification("Item removed from cart", "#E74C3C");
}

// ==================== COUPON SYSTEM ====================
void on_apply_coupon(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    const char *code = gtk_entry_get_text(GTK_ENTRY(coupon_entry));
    if (!code || strlen(code) == 0) {
        show_notification("Please enter a coupon code!", "#E74C3C"); return;
    }
    if (total_bill == 0) {
        show_notification("Add items to cart first!", "#E67E22"); return;
    }
    int disc = check_coupon(code, total_bill);
    if (disc > 0) {
        discount_percent = disc;
        mark_coupon_used(code);
        update_total_display();
        char msg[128];
        sprintf(msg, "Coupon applied! %d%% discount!", disc);
        show_notification(msg, "#27AE60");
    } else {
        show_notification("Invalid or expired coupon!", "#E74C3C");
    }
}

void on_remove_coupon(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    discount_percent = 0;
    discount_amount  = 0;
    gtk_entry_set_text(GTK_ENTRY(coupon_entry), "");
    update_total_display();
    show_notification("Coupon removed", "#E67E22");
}

// ==================== ORDER TYPE ====================
void on_select_dine(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    strcpy(current_order_type, "Dine-In");
    show_notification("Dine-In selected", "#3498DB");
}
void on_select_take(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    strcpy(current_order_type, "Takeaway");
    show_notification("Takeaway selected", "#9B59B6");
}

// ==================== RECEIPT GENERATOR ====================
void generate_receipt(const char *order_id, int subtotal, int discount, int final_total, const char *payment) {
    char filename[64];
    sprintf(filename, "receipt_%s.txt", order_id + 1);
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
    if (discount > 0) fprintf(f, "%-25s %14d\n", "Discount:", -discount);
    fprintf(f, "%-25s %14d\n", "TOTAL PAYABLE:", final_total);
    fprintf(f, "========================================\n");
    fprintf(f, "   Thank you for choosing Burhan Panda!\n");
    fprintf(f, "========================================\n");
    fclose(f);
}

// ==================== PAYMENT SELECTION DIALOG ====================
int show_payment_dialog() {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Select Payment Method", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        "Cancel",  GTK_RESPONSE_CANCEL,
        "Confirm", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), SC(380), SC(260));
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(12));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(20));
    GtkWidget *lbl = gtk_label_new("Choose Payment Method:");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    GtkWidget *rb_cash = gtk_radio_button_new_with_label(NULL, "Cash on Delivery");
    GtkWidget *rb_card = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "Credit / Debit Card");
    GtkWidget *rb_jazz = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "JazzCash");
    GtkWidget *rb_easy = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb_cash), "EasyPaisa");
    gtk_box_pack_start(GTK_BOX(box), lbl,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), rb_cash, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), rb_card, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), rb_jazz, FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), rb_easy, FALSE, FALSE, SC(4));
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        if      (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_card))) strcpy(current_payment, "Card");
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_jazz))) strcpy(current_payment, "JazzCash");
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_easy))) strcpy(current_payment, "EasyPaisa");
        else strcpy(current_payment, "Cash");
    }
    gtk_widget_destroy(dialog);
    return response;
}

// ==================== FEEDBACK DIALOG ====================
void show_feedback_dialog(const char *order_id) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Rate Your Experience", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        "Submit", GTK_RESPONSE_OK,
        "Skip",   GTK_RESPONSE_CANCEL,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), SC(400), SC(260));
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(14));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(22));
    GtkWidget *lbl = gtk_label_new("How was your experience? (1=Poor, 5=Excellent)");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *rl  = gtk_label_new("Rating:");
    GtkWidget *spin = gtk_spin_button_new_with_range(1, 5, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 5);
    gtk_box_pack_start(GTK_BOX(row), rl,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), spin, FALSE, FALSE, 0);
    GtkWidget *cl = gtk_label_new("Comments (optional):");
    gtk_widget_set_halign(cl, GTK_ALIGN_START);
    GtkWidget *tv = gtk_text_view_new();
    gtk_widget_set_size_request(tv, SC(340), SC(60));
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), cl,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), tv,  TRUE,  TRUE,  0);
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int rating = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
        char sql[256];
        sprintf(sql, "UPDATE orders SET feedback=%d WHERE id='%s';", rating, order_id);
        sqlite3_exec(db, sql, 0, 0, 0);
        show_notification("Thank you for your feedback!", "#F39C12");
    }
    gtk_widget_destroy(dialog);
}

// ==================== ORDER STATUS TRACKER ====================
void show_order_tracker(const char *order_id) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Order Status Tracker", GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        "Close", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), SC(500), SC(280));
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(16));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(22));
    GtkWidget *title = gtk_label_new(NULL);
    char markup[128];
    sprintf(markup, "<b>Order %s  |  %s</b>", order_id, current_order_type);
    gtk_label_set_markup(GTK_LABEL(title), markup);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    GtkWidget *steps_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(8));
    const char *steps[]       = {"Received", "Confirmed", "Preparing", "Ready", "Delivered"};
    const char *step_classes[] = {"progress-step-done","progress-step-done","progress-step-active","progress-step-todo","progress-step-todo"};
    for (int i = 0; i < 5; i++) {
        GtkWidget *sl = gtk_label_new(steps[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(sl), step_classes[i]);
        gtk_box_pack_start(GTK_BOX(steps_box), sl, FALSE, FALSE, 0);
        if (i < 4) {
            GtkWidget *arr = gtk_label_new(" > ");
            gtk_box_pack_start(GTK_BOX(steps_box), arr, FALSE, FALSE, 0);
        }
    }
    GtkWidget *progress = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.4);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "Preparing your order...");
    GtkWidget *eta_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(eta_lbl), "<b>Estimated Time: 20-30 mins</b>");
    gtk_widget_set_halign(eta_lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), title,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), steps_box, FALSE, FALSE, SC(8));
    gtk_box_pack_start(GTK_BOX(box), progress,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), eta_lbl,   FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ==================== CHECKOUT ====================
void on_checkout(GtkWidget *b, gpointer d) {
    (void)b; (void)d;
    if (total_bill == 0) {
        show_notification("Cart is empty!", "#E74C3C"); return;
    }
    int pay_response = show_payment_dialog();
    if (pay_response == GTK_RESPONSE_CANCEL) return;

    char order_id[16];
    sprintf(order_id, "#BK%04d", 1000 + rand() % 9000);
    int subtotal    = total_bill;
    int disc        = (total_bill * discount_percent) / 100;
    int final_total = subtotal - disc;

    save_order_to_db(order_id, final_total, current_payment, current_order_type);

    if (order_history_store) {
        GtkTreeIter iter;
        char tot_str[32];
        snprintf(tot_str, sizeof(tot_str), "Rs.%d", final_total);
        gtk_list_store_append(order_history_store, &iter);
        gtk_list_store_set(order_history_store, &iter,
            0, order_id, 1, "Just Now", 2, "Preparing", 3, current_payment, 4, tot_str, -1);
    }

    generate_receipt(order_id, subtotal, disc, final_total, current_payment);

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Order Confirmed!");
    char secondary[512];
    sprintf(secondary,
        "Order ID   : %s\nOrder Type : %s\nPayment    : %s\n"
        "Subtotal   : Rs. %d\nDiscount   : Rs. %d\nTotal Paid : Rs. %d\n\n"
        "Receipt saved as: receipt_%s.txt",
        order_id, current_order_type, current_payment,
        subtotal, disc, final_total, order_id+1);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // Reset cart
    memset(cart_items, 0, sizeof(cart_items));
    cart_item_count  = 0;
    total_bill       = 0;
    discount_percent = 0;
    discount_amount  = 0;
    gtk_list_store_clear(cart_store);
    gtk_entry_set_text(GTK_ENTRY(coupon_entry), "");
    update_total_display();

    // Update analytics
    if (analytics_orders_lbl && analytics_revenue_lbl && analytics_avg_lbl) {
        int total_ords = get_total_orders();
        int total_rev  = get_total_revenue();
        int avg        = total_ords > 0 ? total_rev / total_ords : 0;
        char buf[64];
        sprintf(buf, "%d",      total_ords); gtk_label_set_text(GTK_LABEL(analytics_orders_lbl),  buf);
        sprintf(buf, "Rs. %d",  total_rev);  gtk_label_set_text(GTK_LABEL(analytics_revenue_lbl), buf);
        sprintf(buf, "Rs. %d",  avg);        gtk_label_set_text(GTK_LABEL(analytics_avg_lbl),     buf);
    }

    show_order_tracker(order_id);
    show_feedback_dialog(order_id);
}

// ==================== SEARCH & FILTER ====================
void on_search_changed(GtkEditable *editable, gpointer user_data) {
    (void)user_data;
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    refresh_menu(text);
}

void on_category_click(GtkWidget *btn, gpointer data) {
    (void)btn;
    current_category = (char *)data;
    const char *search_text = search_entry ? gtk_entry_get_text(GTK_ENTRY(search_entry)) : "";
    refresh_menu(search_text);
}

// ==================== FAVORITES TOGGLE ====================
typedef struct { int inv_idx; GtkWidget *fav_btn; } FavData;

void on_toggle_favorite(GtkWidget *btn, gpointer data) {
    FavData *fd = (FavData *)data;
    toggle_favorite(inventory[fd->inv_idx].name);
    GtkStyleContext *ctx = gtk_widget_get_style_context(btn);
    if (is_favorite(inventory[fd->inv_idx].name)) {
        gtk_button_set_label(GTK_BUTTON(fd->fav_btn), "Saved");
        gtk_style_context_add_class(ctx, "saved");
        show_notification("Added to favorites!", "#E21B70");
    } else {
        gtk_button_set_label(GTK_BUTTON(fd->fav_btn), "Favorite");
        gtk_style_context_remove_class(ctx, "saved");
        show_notification("Removed from favorites", "#888888");
    }
}

// ==================== MENU REFRESH ====================
void refresh_menu(const char *search_query) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu_flowbox));
    for (GList *it = children; it != NULL; it = g_list_next(it))
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(children);

    int count = 0;
    for (int i = 0; i < 20; i++) {
        int cat_match = (strcmp(current_category, "All Items") == 0 ||
                         strcmp(inventory[i].category, current_category) == 0);
        int fav_match = (strcmp(current_category, "Favorites") == 0 &&
                         is_favorite(inventory[i].name));
        int search_match = 1;
        if (search_query && strlen(search_query) > 0) {
            gchar *nl = g_utf8_strdown(inventory[i].name, -1);
            gchar *ql = g_utf8_strdown(search_query, -1);
            search_match = (g_strstr_len(nl, -1, ql) != NULL);
            g_free(nl); g_free(ql);
        }

        if ((cat_match || fav_match) && search_match) {
            // ---- BUILD CARD ----
            GtkWidget *card = gtk_button_new();
            gtk_style_context_add_class(gtk_widget_get_style_context(card), "product-card");
            // Card size responsive
            gtk_widget_set_size_request(card, SC(190), SC(255));

            GtkWidget *cv = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(6));
            gtk_container_set_border_width(GTK_CONTAINER(cv), SC(6));

            // Image
            GtkWidget *img = create_product_image(inventory[i].img_path);
            gtk_widget_set_halign(img, GTK_ALIGN_CENTER);

            // Item name — NOW VISIBLE with correct CSS class
            GtkWidget *name_lbl = gtk_label_new(inventory[i].name);
            gtk_style_context_add_class(gtk_widget_get_style_context(name_lbl), "item-name");
            gtk_label_set_line_wrap(GTK_LABEL(name_lbl), TRUE);
            gtk_label_set_max_width_chars(GTK_LABEL(name_lbl), 18);
            gtk_label_set_justify(GTK_LABEL(name_lbl), GTK_JUSTIFY_CENTER);
            gtk_widget_set_halign(name_lbl, GTK_ALIGN_CENTER);

            // Price
            char p[32];
            sprintf(p, "Rs. %d", inventory[i].price);
            GtkWidget *price_lbl = gtk_label_new(p);
            gtk_style_context_add_class(gtk_widget_get_style_context(price_lbl), "price-tag");
            gtk_widget_set_halign(price_lbl, GTK_ALIGN_CENTER);

            // Favorite button
            FavData *fd = g_new(FavData, 1);
            fd->inv_idx = i;
            GtkWidget *fav_btn = gtk_button_new_with_label(
                is_favorite(inventory[i].name) ? "Saved" : "Favorite");
            fd->fav_btn = fav_btn;
            gtk_style_context_add_class(gtk_widget_get_style_context(fav_btn), "btn-fav");
            if (is_favorite(inventory[i].name))
                gtk_style_context_add_class(gtk_widget_get_style_context(fav_btn), "saved");
            g_signal_connect(fav_btn, "clicked", G_CALLBACK(on_toggle_favorite), fd);
            // Free fd when button destroyed
            g_object_set_data_full(G_OBJECT(fav_btn), "favdata", fd, g_free);

            gtk_box_pack_start(GTK_BOX(cv), img,       TRUE,  TRUE,  0);
            gtk_box_pack_start(GTK_BOX(cv), name_lbl,  FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(cv), price_lbl, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(cv), fav_btn,   FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(card), cv);
            g_signal_connect(card, "clicked", G_CALLBACK(on_add_to_cart), GINT_TO_POINTER(i));

            gtk_flow_box_insert(GTK_FLOW_BOX(menu_flowbox), card, -1);
            count++;
        }
    }

    if (count == 0) {
        GtkWidget *empty_lbl = gtk_label_new("No items found.");
        gtk_widget_set_halign(empty_lbl, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(empty_lbl, SC(40));
        gtk_flow_box_insert(GTK_FLOW_BOX(menu_flowbox), empty_lbl, -1);
    }

    gtk_widget_show_all(menu_flowbox);
}

// ==================== NAVIGATION ====================
void on_nav_click(GtkWidget *b, gpointer page_id) {
    (void)b;
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), (const char *)page_id);
}

// ==================== PROFILE HANDLERS ====================
void on_image_upload_clicked(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Profile Picture",
        GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open",   GTK_RESPONSE_ACCEPT, NULL);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image Files");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(filename, SC(110), SC(110), TRUE, NULL);
        if (pb && profile_pixbuf_widget) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(profile_pixbuf_widget), pb);
            g_object_unref(pb);
        }
        g_free(filename);
        show_notification("Profile photo updated!", "#27AE60");
    }
    gtk_widget_destroy(dialog);
}

void on_profile_update(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    show_notification("Profile saved successfully!", "#27AE60");
}

// ==================== DARK MODE (PROPERLY FIXED) ====================
// GTK3 mein dark mode ka sahi tarika:
// GtkSettings se prefer-dark set karo AND apna CSS reload karo
void on_dark_mode_toggled(GtkSwitch *sw, gboolean state, gpointer user_data) {
    (void)sw; (void)user_data;
    dark_mode_on = state;

    // Step 1: GTK3 system dark mode hint
    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings,
        "gtk-application-prefer-dark-theme", state,
        NULL);

    // Step 2: Reload our custom CSS with new dark/light colors
    load_css();

    show_notification(state ? "Dark Mode ON" : "Light Mode ON",
                      state ? "#1A1A2E" : "#F39C12");
}

// ==================== LOGIN SCREEN ====================
static GtkWidget *login_user_entry = NULL;
static GtkWidget *login_pass_entry = NULL;

void on_login_submit(GtkWidget *btn, gpointer data) {
    (void)btn; (void)data;
    const char *user = gtk_entry_get_text(GTK_ENTRY(login_user_entry));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(login_pass_entry));

    if (!user || strlen(user) == 0 || !pass || strlen(pass) == 0) {
        GtkWidget *e = gtk_message_dialog_new(GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Please enter username and password.");
        gtk_dialog_run(GTK_DIALOG(e)); gtk_widget_destroy(e);
        return;
    }

    if (verify_login(user, pass)) {
        strncpy(logged_in_user, user, 63);
        logged_in_user[63] = '\0';
        gtk_stack_set_visible_child_name(GTK_STACK(login_stack), "app");
        char msg[64];
        snprintf(msg, sizeof(msg), "Welcome back, %s!", logged_in_user);
        show_notification(msg, "#E21B70");
    } else {
        GtkWidget *err = gtk_message_dialog_new(GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Login Failed");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(err),
            "Invalid username or password!\nDefault: admin / 1234");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
    }
}

GtkWidget *create_login_page() {
    GtkWidget *bg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(center_box, TRUE);
    gtk_widget_set_hexpand(center_box, TRUE);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(14));
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "login-card");
    gtk_widget_set_size_request(card, SC(400), -1);
    gtk_container_set_border_width(GTK_CONTAINER(card), SC(35));

    GtkWidget *logo = gtk_label_new("Burhan Panda");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "login-title");
    GtkWidget *sub = gtk_label_new("Food Management System - v2.0");
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "login-sub");
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    GtkWidget *ul = gtk_label_new("Username:");
    gtk_widget_set_halign(ul, GTK_ALIGN_START);
    login_user_entry = gtk_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(login_user_entry), "login-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(login_user_entry), "Enter username");

    GtkWidget *pl = gtk_label_new("Password:");
    gtk_widget_set_halign(pl, GTK_ALIGN_START);
    login_pass_entry = gtk_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(login_pass_entry), "login-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(login_pass_entry), "Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(login_pass_entry), FALSE);
    g_signal_connect(login_pass_entry, "activate", G_CALLBACK(on_login_submit), NULL);

    GtkWidget *lbtn = gtk_button_new_with_label("Login");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbtn), "login-btn");
    gtk_widget_set_size_request(lbtn, -1, 44);
    g_signal_connect(lbtn, "clicked", G_CALLBACK(on_login_submit), NULL);

    GtkWidget *hint = gtk_label_new("Default: admin / 1234");
    gtk_style_context_add_class(gtk_widget_get_style_context(hint), "login-hint");
    gtk_widget_set_halign(hint, GTK_ALIGN_CENTER);

    gtk_box_pack_start(GTK_BOX(card), logo,            FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), sub,             FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), sep,             FALSE, FALSE, SC(8));
    gtk_box_pack_start(GTK_BOX(card), ul,              FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), login_user_entry,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), pl,              FALSE, FALSE, SC(6));
    gtk_box_pack_start(GTK_BOX(card), login_pass_entry,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), lbtn,            FALSE, FALSE, SC(10));
    gtk_box_pack_start(GTK_BOX(card), hint,            FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_box), card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bg), center_box, TRUE, TRUE, 0);
    return bg;
}

// ==================== PAGE: DASHBOARD ====================
GtkWidget *create_dashboard() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(12));
    gtk_container_set_border_width(GTK_CONTAINER(vbox), SC(14));
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    // ---- CATEGORY BAR — horizontal scroll so ALL categories visible ----
    GtkWidget *cat_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cat_scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);  // horizontal scroll allowed
    gtk_widget_set_size_request(cat_scroll, -1, SC(44));
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cat_scroll), GTK_SHADOW_NONE);

    GtkWidget *cat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(5));
    gtk_container_set_border_width(GTK_CONTAINER(cat_box), SC(3));

    // All 9 categories — using static strings (safe, no dangling pointer)
    static const char *cat_labels[] = {
        "All Items","Burgers","Pizza","Pasta","BBQ","Rice","Drinks","Desserts","Favorites"
    };
    // We pass index and use cat_labels in on_category_click
    // But original code passes pointer — keep same approach with static strings
    for (int i = 0; i < 9; i++) {
        GtkWidget *btn = gtk_button_new_with_label(cat_labels[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "category-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_category_click),
                         (gpointer)cat_labels[i]);
        gtk_box_pack_start(GTK_BOX(cat_box), btn, FALSE, FALSE, 0);
    }
    gtk_container_add(GTK_CONTAINER(cat_scroll), cat_box);
    gtk_box_pack_start(GTK_BOX(vbox), cat_scroll, FALSE, FALSE, 0);

    // ---- MENU using FlowBox — auto wraps based on available width ----
    menu_flowbox = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(menu_flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(menu_flowbox), TRUE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(menu_flowbox), SC(14));
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(menu_flowbox), SC(14));
    // min-children-per-line = how many columns min
    // max-children-per-line = how many max (auto fills screen)
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(menu_flowbox), 2);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(menu_flowbox), 8);
    gtk_widget_set_hexpand(menu_flowbox, TRUE);

    refresh_menu("");
    gtk_box_pack_start(GTK_BOX(vbox), menu_flowbox, TRUE, TRUE, 0);
    return scroll;
}

// ==================== PAGE: ORDERS ====================
GtkWidget *create_orders_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(14));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(24));

    GtkWidget *title = gtk_label_new("Order History");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "page-title");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    // 5 columns: ID, Date, Status, Payment, Total
    order_history_store = gtk_list_store_new(5,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(order_history_store));
    order_history_tree = tree;
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree), GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    g_object_set(r, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    const char *hdrs[] = {"Order ID","Date","Status","Payment","Total"};
    int col_widths[]   = {SC(90), SC(130), SC(95), SC(90), SC(80)};
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

    GtkWidget *btn_row   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *track_btn = gtk_button_new_with_label("Track Latest Order");
    gtk_style_context_add_class(gtk_widget_get_style_context(track_btn), "btn-success");
    g_signal_connect_swapped(track_btn, "clicked",
        G_CALLBACK(show_order_tracker), (gpointer)"#LATEST");
    gtk_box_pack_start(GTK_BOX(btn_row), track_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), title,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), scroll,  TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(box), btn_row, FALSE, FALSE, 0);
    return box;
}

// ==================== PAGE: ANALYTICS ====================
static void refresh_analytics_stats() {
    if (!analytics_orders_lbl) return;
    int total_ords = get_total_orders();
    int total_rev  = get_total_revenue();
    int avg        = total_ords > 0 ? total_rev / total_ords : 0;
    char buf[64];
    sprintf(buf, "%d",     total_ords); gtk_label_set_text(GTK_LABEL(analytics_orders_lbl),  buf);
    sprintf(buf, "Rs. %d", total_rev);  gtk_label_set_text(GTK_LABEL(analytics_revenue_lbl), buf);
    sprintf(buf, "Rs. %d", avg);        gtk_label_set_text(GTK_LABEL(analytics_avg_lbl),     buf);
}

GtkWidget *create_analytics_page() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(18));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(24));
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *title = gtk_label_new("Analytics Dashboard");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "page-title");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    // Stat cards
    GtkWidget *stats_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(16));

    struct {
        const char *icon; const char *lbl; GtkWidget **ref; const char *init;
    } cards[] = {
        {"Orders",  "Total Orders",    &analytics_orders_lbl,  "0"},
        {"Revenue", "Total Revenue",   &analytics_revenue_lbl, "Rs.0"},
        {"Average", "Avg Order Value", &analytics_avg_lbl,     "Rs.0"},
        {"Items",   "Menu Items",      NULL,                   "20"},
    };

    for (int i = 0; i < 4; i++) {
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(6));
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "stat-card");
        gtk_widget_set_size_request(card, SC(155), SC(105));
        gtk_container_set_border_width(GTK_CONTAINER(card), SC(12));
        GtkWidget *ic  = gtk_label_new(cards[i].icon);
        GtkWidget *num = gtk_label_new(cards[i].init);
        gtk_style_context_add_class(gtk_widget_get_style_context(num), "stat-number");
        GtkWidget *lb  = gtk_label_new(cards[i].lbl);
        gtk_style_context_add_class(gtk_widget_get_style_context(lb), "stat-label");
        if (cards[i].ref) *(cards[i].ref) = num;
        gtk_widget_set_halign(ic,  GTK_ALIGN_START);
        gtk_widget_set_halign(num, GTK_ALIGN_START);
        gtk_widget_set_halign(lb,  GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(card), ic,  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), num, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), lb,  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(stats_row), card, FALSE, FALSE, 0);
    }

    // Category distribution chart
    GtkWidget *chart_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(chart_title), "<b>Category Distribution</b>");
    gtk_widget_set_halign(chart_title, GTK_ALIGN_START);

    GtkWidget *chart_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(7));
    gtk_style_context_add_class(gtk_widget_get_style_context(chart_box), "stat-card");
    gtk_container_set_border_width(GTK_CONTAINER(chart_box), SC(14));

    const char *cats[]    = {"Burgers","Pizza","Pasta","BBQ","Rice","Drinks","Desserts"};
    int percentages[]     = {20, 18, 15, 12, 12, 13, 10};
    for (int i = 0; i < 7; i++) {
        GtkWidget *row     = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
        GtkWidget *cat_lbl = gtk_label_new(cats[i]);
        gtk_widget_set_size_request(cat_lbl, SC(75), -1);
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

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh Stats");
    gtk_style_context_add_class(gtk_widget_get_style_context(refresh_btn), "btn-success");
    g_signal_connect_swapped(refresh_btn, "clicked",
        G_CALLBACK(refresh_analytics_stats), NULL);

    gtk_box_pack_start(GTK_BOX(box), title,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), stats_row,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), chart_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), chart_box,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), refresh_btn, FALSE, FALSE, 0);

    refresh_analytics_stats();
    return scroll;
}

// ==================== PAGE: PROFILE ====================
GtkWidget *create_profile_page() {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(12));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(30));
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *title = gtk_label_new("Profile Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "page-title");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    profile_pixbuf_widget = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_halign(profile_pixbuf_widget, GTK_ALIGN_START);

    GtkWidget *img_btn = gtk_button_new_with_label("Upload Photo");
    gtk_style_context_add_class(gtk_widget_get_style_context(img_btn), "category-btn");
    g_signal_connect(img_btn, "clicked", G_CALLBACK(on_image_upload_clicked), NULL);

    GtkWidget *fg = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(fg), SC(8));
    gtk_grid_set_column_spacing(GTK_GRID(fg), SC(12));

    struct {
        const char *lbl; const char *ph; gboolean pw;
    } fields[] = {
        {"Full Name:",    "Enter full name",   FALSE},
        {"Email:",        "Email address",     FALSE},
        {"Phone:",        "+92 3XX XXXXXXX",   FALSE},
        {"Address:",      "Delivery address",  FALSE},
        {"Old Password:", "Current password",  TRUE},
        {"New Password:", "New password",       TRUE},
    };
    for (int i = 0; i < 6; i++) {
        GtkWidget *l = gtk_label_new(fields[i].lbl);
        gtk_widget_set_halign(l, GTK_ALIGN_END);
        GtkWidget *e = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(e), fields[i].ph);
        if (fields[i].pw) gtk_entry_set_visibility(GTK_ENTRY(e), FALSE);
        gtk_widget_set_hexpand(e, TRUE);
        if (i == 0) gtk_entry_set_text(GTK_ENTRY(e), "Burhan Ahmed");
        gtk_grid_attach(GTK_GRID(fg), l, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(fg), e, 1, i, 1, 1);
    }

    GtkWidget *save = gtk_button_new_with_label("Save Profile");
    gtk_style_context_add_class(gtk_widget_get_style_context(save), "btn-checkout");
    g_signal_connect(save, "clicked", G_CALLBACK(on_profile_update), NULL);

    gtk_box_pack_start(GTK_BOX(box), title,               FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), profile_pixbuf_widget,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), img_btn,             FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, SC(4));
    gtk_box_pack_start(GTK_BOX(box), fg,                  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), save,                FALSE, FALSE, SC(10));
    return scroll;
}

// ==================== PAGE: SETTINGS ====================
GtkWidget *create_settings_page(GtkWidget *win) {
    (void)win;  // We use global dark_mode_on now
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(16));
    gtk_container_set_border_width(GTK_CONTAINER(box), SC(30));
    gtk_container_add(GTK_CONTAINER(scroll), box);

    GtkWidget *title = gtk_label_new("App Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "page-title");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    // Dark mode toggle
    GtkWidget *dm_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *dm_lbl = gtk_label_new("Dark Mode");
    gtk_widget_set_hexpand(dm_lbl, TRUE);
    gtk_widget_set_halign(dm_lbl, GTK_ALIGN_START);
    GtkWidget *dm_sw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(dm_sw), dark_mode_on);
    // on_dark_mode_toggled handles everything — no window param needed
    g_signal_connect(dm_sw, "state-set", G_CALLBACK(on_dark_mode_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(dm_row), dm_lbl, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(dm_row), dm_sw,  FALSE, FALSE, 0);

    // Notifications toggle (UI element, functional)
    GtkWidget *notif_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *notif_lbl = gtk_label_new("Show Notifications");
    gtk_widget_set_hexpand(notif_lbl, TRUE);
    gtk_widget_set_halign(notif_lbl, GTK_ALIGN_START);
    GtkWidget *notif_sw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(notif_sw), TRUE);
    gtk_box_pack_start(GTK_BOX(notif_row), notif_lbl, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(notif_row), notif_sw,  FALSE, FALSE, 0);

    // Reset coupons
    GtkWidget *rc_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(10));
    GtkWidget *rc_lbl = gtk_label_new("Reset All Coupons (testing)");
    gtk_widget_set_hexpand(rc_lbl, TRUE);
    gtk_widget_set_halign(rc_lbl, GTK_ALIGN_START);
    GtkWidget *rc_btn = gtk_button_new_with_label("Reset Coupons");
    gtk_style_context_add_class(gtk_widget_get_style_context(rc_btn), "btn-success");
    g_signal_connect_swapped(rc_btn, "clicked", G_CALLBACK(coupon_reset_all), NULL);
    gtk_box_pack_start(GTK_BOX(rc_row), rc_lbl, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(rc_row), rc_btn, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    // Coupon info
    GtkWidget *cp_info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(cp_info),
        "<b>Available Coupons:</b>\n"
        "  PANDA10  =  10%% off  (min Rs.500)\n"
        "  SAVE20   =  20%% off  (min Rs.1000)\n"
        "  FLAT50   =  50%% off  (min Rs.2000)");
    gtk_widget_set_halign(cp_info, GTK_ALIGN_START);

    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    GtkWidget *about = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(about),
        "<b>Burhan Panda v2.0</b>\n"
        "<small>GTK3 + SQLite3  |  C Language\n"
        "Auto-responsive: Laptop and Projector\n"
        "Login | Cart | Coupons | Analytics | Favorites\n"
        "Order Tracking | Receipts | Dark Mode</small>");
    gtk_widget_set_halign(about, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(box), title,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), dm_row,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), notif_row,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), rc_row,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sep,      FALSE, FALSE, SC(5));
    gtk_box_pack_start(GTK_BOX(box), cp_info,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sep2,     FALSE, FALSE, SC(5));
    gtk_box_pack_start(GTK_BOX(box), about,    FALSE, FALSE, 0);
    return scroll;
}

// ==================== MAIN APP UI ASSEMBLY ====================
GtkWidget *build_app_ui() {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // ---- TOP HEADER ----
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(12));
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "top-header");

    GtkWidget *logo = gtk_label_new("Burhan Panda  🐼");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "logo-text");
    gtk_widget_set_halign(logo, GTK_ALIGN_START);

    GtkWidget *hdr_spacer = gtk_label_new("");
    gtk_widget_set_hexpand(hdr_spacer, TRUE);

    // Search entry — class on entry directly for GTK3 CSS targeting
    search_entry = gtk_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "search-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search for foods...");
    gtk_widget_set_size_request(search_entry, SC(280), -1);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(search_entry),
        GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);

    // User badge
    GtkWidget *user_lbl = gtk_label_new(logged_in_user);
    gtk_style_context_add_class(gtk_widget_get_style_context(user_lbl), "user-badge");

    gtk_box_pack_start(GTK_BOX(header), logo,         FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), hdr_spacer,   TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(header), search_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), user_lbl,     FALSE, FALSE, SC(8));
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    // ---- NOTIFICATION REVEALER ----
    notif_revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(notif_revealer),
        GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(GTK_REVEALER(notif_revealer), 250);
    notif_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(notif_label), "notif-text");
    gtk_widget_set_halign(notif_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(notif_label, TRUE);
    // notif_box_inner is the colored container
    notif_box_inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(notif_box_inner), "notification-bar");
    gtk_box_pack_start(GTK_BOX(notif_box_inner), notif_label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(notif_revealer), notif_box_inner);
    gtk_box_pack_start(GTK_BOX(root), notif_revealer, FALSE, FALSE, 0);

    // ---- BODY (sidebar + stack + cart) ----
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(root), main_hbox, TRUE, TRUE, 0);

    // ---- SIDEBAR ----
    GtkWidget *side = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(3));
    gtk_style_context_add_class(gtk_widget_get_style_context(side), "sidebar");
    // Fixed compact width — tidak terlalu besar
    gtk_widget_set_size_request(side, SC(145), -1);
    gtk_container_set_border_width(GTK_CONTAINER(side), SC(8));

    const char *nav_items[] = {"🏠  Dashboard", "📋  Orders", "📊  Analytics", "👤  Profile", "⚙️  Settings"};
    const char *nav_ids[]   = {"dash", "orders", "analytics", "prof", "set"};
    for (int i = 0; i < 5; i++) {
        GtkWidget *b = gtk_button_new_with_label(nav_items[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(b), "nav-btn");
        gtk_widget_set_halign(b, GTK_ALIGN_FILL);
        g_signal_connect(b, "clicked", G_CALLBACK(on_nav_click), (gpointer)nav_ids[i]);
        gtk_box_pack_start(GTK_BOX(side), b, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(main_hbox), side, FALSE, FALSE, 0);

    // ---- MAIN STACK (takes ALL remaining width) ----
    main_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(main_stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(main_stack), 180);
    gtk_widget_set_hexpand(main_stack, TRUE);   // KEY FIX: stack expands, cart stays fixed

    gtk_stack_add_named(GTK_STACK(main_stack), create_dashboard(),         "dash");
    gtk_stack_add_named(GTK_STACK(main_stack), create_orders_page(),       "orders");
    gtk_stack_add_named(GTK_STACK(main_stack), create_analytics_page(),    "analytics");
    gtk_stack_add_named(GTK_STACK(main_stack), create_profile_page(),      "prof");
    gtk_stack_add_named(GTK_STACK(main_stack), create_settings_page(main_window), "set");
    gtk_box_pack_start(GTK_BOX(main_hbox), main_stack, TRUE, TRUE, 0);

    // ---- CART PANEL ----
    // KEY FIX: compact fixed width — SC(240) = 240px on laptop, 300px on FHD
    // Menu gets all remaining space, cart stays on right as compact panel
    GtkWidget *cart = gtk_box_new(GTK_ORIENTATION_VERTICAL, SC(7));
    gtk_style_context_add_class(gtk_widget_get_style_context(cart), "cart-panel");
    gtk_widget_set_size_request(cart, SC(240), -1);   // Compact! Previously was 320-350
    gtk_widget_set_hexpand(cart, FALSE);               // Do NOT expand
    gtk_container_set_border_width(GTK_CONTAINER(cart), SC(10));

    // Cart title
    GtkWidget *basket_label = gtk_label_new("Your Basket  🛒");
    gtk_style_context_add_class(gtk_widget_get_style_context(basket_label), "cart-title");
    gtk_widget_set_halign(basket_label, GTK_ALIGN_START);

    // Order type
    GtkWidget *otype_lbl = gtk_label_new("Order Type:");
    gtk_style_context_add_class(gtk_widget_get_style_context(otype_lbl), "section-lbl");
    gtk_widget_set_halign(otype_lbl, GTK_ALIGN_START);
    GtkWidget *otype_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(6));
    order_type_dine_btn = gtk_button_new_with_label("Dine-In");
    order_type_take_btn = gtk_button_new_with_label("Takeaway");
    gtk_style_context_add_class(gtk_widget_get_style_context(order_type_dine_btn), "category-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(order_type_take_btn), "category-btn");
    g_signal_connect(order_type_dine_btn, "clicked", G_CALLBACK(on_select_dine), NULL);
    g_signal_connect(order_type_take_btn, "clicked", G_CALLBACK(on_select_take), NULL);
    gtk_box_pack_start(GTK_BOX(otype_row), order_type_dine_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(otype_row), order_type_take_btn, TRUE, TRUE, 0);

    // Cart treeview — Name | Qty | Price
    // 3 columns with proper string types
    cart_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    cart_view  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cart_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(cart_view), TRUE);
    g_signal_connect(cart_view, "row-activated", G_CALLBACK(on_remove_item), NULL);

    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    g_object_set(r, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    GtkTreeViewColumn *c0 = gtk_tree_view_column_new_with_attributes("Item", r, "text", 0, NULL);
    GtkTreeViewColumn *c1 = gtk_tree_view_column_new_with_attributes("Qty",  r, "text", 1, NULL);
    GtkTreeViewColumn *c2 = gtk_tree_view_column_new_with_attributes("Rs.",  r, "text", 2, NULL);
    gtk_tree_view_column_set_expand(c0, TRUE);        // Item name column expands
    gtk_tree_view_column_set_min_width(c1, SC(35));
    gtk_tree_view_column_set_min_width(c2, SC(60));
    gtk_tree_view_append_column(GTK_TREE_VIEW(cart_view), c0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cart_view), c1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cart_view), c2);

    GtkWidget *cs = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(cs, TRUE);
    gtk_container_add(GTK_CONTAINER(cs), cart_view);

    GtkWidget *hint_lbl = gtk_label_new("(double-click to remove)");
    gtk_style_context_add_class(gtk_widget_get_style_context(hint_lbl), "hint-lbl");
    gtk_widget_set_halign(hint_lbl, GTK_ALIGN_CENTER);

    // Coupon section
    GtkWidget *coupon_lbl = gtk_label_new("Coupon:");
    gtk_style_context_add_class(gtk_widget_get_style_context(coupon_lbl), "section-lbl");
    gtk_widget_set_halign(coupon_lbl, GTK_ALIGN_START);
    GtkWidget *coupon_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SC(4));
    coupon_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(coupon_entry), "PANDA10");
    gtk_widget_set_hexpand(coupon_entry, TRUE);
    GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
    gtk_style_context_add_class(gtk_widget_get_style_context(apply_btn), "btn-success");
    GtkWidget *rem_btn = gtk_button_new_with_label("X");
    gtk_style_context_add_class(gtk_widget_get_style_context(rem_btn), "btn-danger");
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_apply_coupon), NULL);
    g_signal_connect(rem_btn,   "clicked", G_CALLBACK(on_remove_coupon), NULL);
    gtk_box_pack_start(GTK_BOX(coupon_row), coupon_entry, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(coupon_row), apply_btn,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(coupon_row), rem_btn,      FALSE, FALSE, 0);

    // Discount label (hidden by default)
    discount_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(discount_label), "discount-lbl");
    gtk_widget_set_visible(discount_label, FALSE);
    gtk_widget_set_halign(discount_label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(discount_label), TRUE);

    // Total amount
    total_label = gtk_label_new("Rs. 0");
    gtk_style_context_add_class(gtk_widget_get_style_context(total_label), "total-lbl");
    gtk_widget_set_halign(total_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(total_label, SC(4));

    // Checkout button
    GtkWidget *checkout_btn = gtk_button_new_with_label("Checkout");
    gtk_style_context_add_class(gtk_widget_get_style_context(checkout_btn), "btn-checkout");
    gtk_widget_set_hexpand(checkout_btn, TRUE);
    g_signal_connect(checkout_btn, "clicked", G_CALLBACK(on_checkout), NULL);

    // Pack cart panel
    gtk_box_pack_start(GTK_BOX(cart), basket_label,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), otype_lbl,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), otype_row,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), cs,             TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(cart), hint_lbl,      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), coupon_lbl,    FALSE, FALSE, SC(3));
    gtk_box_pack_start(GTK_BOX(cart), coupon_row,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), discount_label,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), total_label,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cart), checkout_btn,  FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_hbox), cart, FALSE, FALSE, 0);
    return root;
}

// ==================== MAIN ====================
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    gtk_init(&argc, &argv);

    // 1. Compute screen-based scale FIRST
    compute_ui_scale();

    // 2. Init database
    init_database();

    // 3. Load CSS (light mode default)
    dark_mode_on = FALSE;
    load_css();

    // 4. Create main window — MAXIMIZED fills laptop/projector screen
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window),
        "Burhan Panda - Food Management v2.0");
    gtk_window_maximize(GTK_WINDOW(main_window));  // Always fills screen
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 5. Login ? App stack
    login_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(login_stack),
        GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_add_named(GTK_STACK(login_stack), create_login_page(), "login");
    gtk_stack_add_named(GTK_STACK(login_stack), build_app_ui(),      "app");
    gtk_stack_set_visible_child_name(GTK_STACK(login_stack), "login");
    gtk_container_add(GTK_CONTAINER(main_window), login_stack);

    gtk_widget_show_all(main_window);
    gtk_main();

    sqlite3_close(db);
    return 0;
}
