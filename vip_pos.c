#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
//                                CONFIG & STRUCTS
// ============================================================================

typedef struct {
    int id;
    char name[100];
    int price;
    char category[50];
} Product;

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *login_box;
    GtkWidget *pos_box;
    GtkWidget *admin_box;
    GtkWidget *cart_store;
    GtkWidget *total_lbl;
    sqlite3 *db;
    int current_bill;
} AppCore;

AppCore *App;

// ============================================================================
//                          DATABASE ENGINE (SQLite)
// ============================================================================

void init_database() {
    int rc = sqlite3_open("burhan_panda.db", &App->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(App->db));
        return;
    }

    char *err_msg = 0;
    char *sql = "CREATE TABLE IF NOT EXISTS Orders("
                "Id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "ItemName TEXT, Price INT, OrderDate DATETIME);";
    
    sqlite3_exec(App->db, sql, 0, 0, &err_msg);
}

void save_order_to_db(const char *name, int price) {
    char sql[256];
    sprintf(sql, "INSERT INTO Orders (ItemName, Price, OrderDate) VALUES ('%s', %d, datetime('now'));", name, price);
    sqlite3_exec(App->db, sql, 0, 0, 0);
}

// ============================================================================
//                          NAVIGATION & LOGIC
// ============================================================================

void on_login_attempt(GtkWidget *btn, gpointer entry_ptr) {
    const char *pass = gtk_entry_get_text(GTK_ENTRY(entry_ptr));
    if (strcmp(pass, "panda123") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(App->stack), "pos_page");
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(App->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Ghalat Password!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

void on_add_to_cart(GtkWidget *btn, gpointer data) {
    Product *p = (Product*)data;
    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(App->cart_store), &iter);
    gtk_list_store_set(GTK_LIST_STORE(App->cart_store), &iter, 0, p->name, 1, p->price, -1);
    
    App->current_bill += p->price;
    gtk_label_set_text(GTK_LABEL(App->total_lbl), g_strdup_printf("Total: Rs. %d", App->current_bill));
    
    save_order_to_db(p->name, p->price); // Permanent Database Storage
}

// ============================================================================
//                          UI MODULES (HEAVY)
// ============================================================================

GtkWidget* build_login_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

    GtkWidget *logo = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(logo), "<span size='30000' weight='bold' color='#D70F64'>BURHAN PANDA LOGIN</span>");
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter System Password");
    
    GtkWidget *btn = gtk_button_new_with_label("ACCESS SYSTEM");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "btn-login");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_login_attempt), entry);

    gtk_box_pack_start(GTK_BOX(box), logo, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 10);

    return box;
}

GtkWidget* build_pos_page() {
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    // Sidebar
    GtkWidget *side = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(side, 200, -1);
    gtk_box_pack_start(GTK_BOX(side), gtk_button_new_with_label("Dashboard"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(side), gtk_button_new_with_label("Admin Panel"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_hbox), side, FALSE, FALSE, 15);

    // Products Grid
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    
    Product *p1 = malloc(sizeof(Product)); strcpy(p1->name, "Zinger Burger"); p1->price = 650;
    GtkWidget *btn1 = gtk_button_new_with_label("Zinger - 650");
    g_signal_connect(btn1, "clicked", G_CALLBACK(on_add_to_cart), p1);
    gtk_grid_attach(GTK_GRID(grid), btn1, 0, 0, 1, 1);

    Product *p2 = malloc(sizeof(Product)); strcpy(p2->name, "Cheese Pizza"); p2->price = 1200;
    GtkWidget *btn2 = gtk_button_new_with_label("Pizza - 1200");
    g_signal_connect(btn2, "clicked", G_CALLBACK(on_add_to_cart), p2);
    gtk_grid_attach(GTK_GRID(grid), btn2, 1, 0, 1, 1);

    gtk_box_pack_start(GTK_BOX(main_hbox), grid, TRUE, TRUE, 20);

    // Cart Section
    GtkWidget *cart_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    App->cart_store = (GtkWidget*)gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(App->cart_store));
    
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Item", r, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Price", r, "text", 1, NULL);
    
    App->total_lbl = gtk_label_new("Total: Rs. 0");
    gtk_box_pack_start(GTK_BOX(cart_vbox), tree, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(cart_vbox), App->total_lbl, FALSE, FALSE, 10);
    
    gtk_box_pack_end(GTK_BOX(main_hbox), cart_vbox, FALSE, FALSE, 15);

    return main_hbox;
}

// ============================================================================
//                                MAIN INIT
// ============================================================================

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    App = malloc(sizeof(AppCore));
    App->current_bill = 0;
    init_database();

    App->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(App->window), 1200, 800);
    g_signal_connect(App->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    App->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(App->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    
    gtk_stack_add_named(GTK_STACK(App->stack), build_login_page(), "login_page");
    gtk_stack_add_named(GTK_STACK(App->stack), build_pos_page(), "pos_page");
    
    gtk_container_add(GTK_CONTAINER(App->window), App->stack);

    // Professional CSS
    const char *css = "button { padding: 10px; font-weight: bold; border-radius: 5px; }"
                      ".btn-login { background: #D70F64; color: white; min-width: 200px; }";
    GtkCssProvider *cp = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cp, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cp), 800);

    gtk_widget_show_all(App->window);
    gtk_main();

    sqlite3_close(App->db);
    return 0;
}
