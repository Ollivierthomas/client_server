#include <gtk/gtk.h>

// On crée une structure pour regrouper nos widgets. 
// Cela permet de les passer facilement aux fonctions (callbacks).
typedef struct {
    GtkWidget *label_historique;
    GtkWidget *champ_saisie;
} ElementsInterface;

// Callback quand on clique sur "Envoyer"
static void on_envoyer_clicked(GtkButton *bouton, gpointer data) {
    // On récupère notre structure
    ElementsInterface *ui = (ElementsInterface *)data;

    // 1. On récupère le texte écrit par l'utilisateur
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(ui->champ_saisie));
    const char *texte_saisi = gtk_entry_buffer_get_text(buffer);

    // Si le texte n'est pas vide, on fait quelque chose
    if (strlen(texte_saisi) > 0) {
        // Pour l'instant, on change juste le texte de l'historique
        // (Dans le vrai chat, c'est ici qu'on enverra le message au serveur)
        gtk_label_set_text(GTK_LABEL(ui->label_historique), texte_saisi);

        // On vide le champ de saisie pour le prochain message
        gtk_entry_buffer_set_text(buffer, "", 0);
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    // On récupère la structure partagée
    ElementsInterface *ui = (ElementsInterface *)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Interface de Chat - Étape 2");
    gtk_window_set_default_size(GTK_WINDOW(window), 350, 400);

    // 1. Création de la Box verticale (espacement de 10 pixels entre les éléments)
    GtkWidget *box_principale = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    // On dit à la fenêtre que son enfant unique est cette Box
    gtk_window_set_child(GTK_WINDOW(window), box_principale);

    // 2. Création du widget pour l'historique (un simple Label pour commencer)
    ui->label_historique = gtk_label_new("L'historique des messages s'affichera ici.");
    // On demande au label de prendre le maximum de place possible (il pousse le reste vers le bas)
    gtk_widget_set_vexpand(ui->label_historique, TRUE); 
    gtk_box_append(GTK_BOX(box_principale), ui->label_historique);

    // 3. Création du champ de saisie de texte
    ui->champ_saisie = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ui->champ_saisie), "Écris ton message...");
    gtk_box_append(GTK_BOX(box_principale), ui->champ_saisie);

    // 4. Création du bouton Envoyer
    GtkWidget *bouton_envoyer = gtk_button_new_with_label("Envoyer");
    // On connecte le signal en passant notre structure 'ui' en paramètre
    g_signal_connect(bouton_envoyer, "clicked", G_CALLBACK(on_envoyer_clicked), ui);
    gtk_box_append(GTK_BOX(box_principale), bouton_envoyer);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    
    // On déclare notre structure
    ElementsInterface ui;

    app = gtk_application_new("org.gtk.etape2", G_APPLICATION_DEFAULT_FLAGS);
    
    // On passe l'adresse de 'ui' à la fonction activate
    g_signal_connect(app, "activate", G_CALLBACK(activate), &ui);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}