#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"
#include <pthread.h>
#include <stdbool.h>
#include <gtk/gtk.h>

// Constantes pour la configuration
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8989
#define BUFFER_SIZE 1024

// Structure pour stocker l'état de l'application
typedef struct {
    int client_socket;           // Socket de connexion au serveur
    GtkTextBuffer *text_buffer;  // Buffer pour afficher les messages
    GtkEntry *entry;             // Champ de saisie des messages
} AppState;

// Données passées au thread pour thread-safety
typedef struct {
    AppState *state;
    char message[BUFFER_SIZE];
} MessageData;

// Variables globales
static volatile bool should_stop = false;        // Flag pour arrêter les threads
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex pour synchronisation
static AppState *app_state = NULL;

// Callback GTK appelé depuis le thread principal pour ajouter un message
// (GTK n'est pas thread-safe, donc on utilise g_idle_add pour exécuter sur le thread principal)
gboolean idle_append_message(gpointer user_data) {
    MessageData *data = (MessageData *)user_data;
    if (!data || !data->state) {
        free(data);
        return FALSE;
    }

    // Insérer le message et une nouvelle ligne dans le buffer de texte
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(data->state->text_buffer, &end);
    gtk_text_buffer_insert(data->state->text_buffer, &end, data->message, -1);
    gtk_text_buffer_insert(data->state->text_buffer, &end, "\n", -1);

    free(data);
    return FALSE;
}

// Ajouter un message de manière thread-safe via g_idle_add
void queue_message(AppState *state, const char *message) {
    MessageData *data = malloc(sizeof(MessageData));
    if (!data) return;

    data->state = state;
    snprintf(data->message, sizeof(data->message), "%s", message);

    // g_idle_add garantit que idle_append_message s'exécute sur le thread GTK principal
    g_idle_add(idle_append_message, data);
}

// Thread de réception des messages depuis le serveur
void* receive_messages_thread(void *user_data) {
    AppState *state = (AppState *)user_data;
    char buffer[BUFFER_SIZE];

    while (1) {
        // Vérifier le flag d'arrêt de manière thread-safe
        pthread_mutex_lock(&state_mutex);
        if (should_stop) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }
        pthread_mutex_unlock(&state_mutex);

        // Recevoir un message du serveur
        if (receive_message(state->client_socket, buffer, sizeof(buffer)) == 0) {
            // Message reçu avec succès, l'afficher
            queue_message(state, buffer);
        } else {
            // Erreur de réception, arrêter le thread
            pthread_mutex_lock(&state_mutex);
            should_stop = true;
            pthread_mutex_unlock(&state_mutex);
            break;
        }
    }
    return NULL;
}

// Callback appelé quand on clique sur le bouton "Send"
void on_send_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(state->entry));

    // Envoyer le message s'il n'est pas vide
    if (text && text[0] != '\0') {
        send_message(state->client_socket, (char *)text);

        // Afficher localement "You: message"
        char display[BUFFER_SIZE];
        snprintf(display, sizeof(display), "You: %s", text);
        queue_message(state, display);

        // Vider le champ de saisie
        gtk_editable_set_text(GTK_EDITABLE(state->entry), "");
    }
}

// Callback pour la touche Entrée dans le champ de texte
gboolean on_key_press(GtkEventControllerKey *controller, guint keyval,
                      guint keycode, GdkModifierType state, gpointer user_data) {
    // Si la touche est Entrée, simuler un clic sur Send
    if (keyval == GDK_KEY_Return) {
        on_send_clicked(NULL, user_data);
        return TRUE;
    }
    return FALSE;
}

// Callback appelé quand on ferme la fenêtre
void on_window_close(GtkWindow *window, gpointer user_data) {
    // Signal l'arrêt de manière thread-safe
    pthread_mutex_lock(&state_mutex);
    should_stop = true;
    pthread_mutex_unlock(&state_mutex);

    // Fermer la socket
    if (app_state) {
        close(app_state->client_socket);
        app_state->client_socket = -1;
    }
}

int main(int argc, char *argv[]) {
    // Créer un socket TCP pour se connecter au serveur
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    // Configurer l'adresse du serveur
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_address.sin_port = htons(SERVER_PORT);

    // Se connecter au serveur
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection to server failed");
        close(client_socket);
        return 1;
    }

    printf("Connected to server!\n");

    // Initialiser GTK
    gtk_init();

    // Allouer et initialiser l'état de l'application
    app_state = malloc(sizeof(AppState));
    if (!app_state) {
        close(client_socket);
        return 1;
    }
    app_state->client_socket = client_socket;

    // === Construction de l'interface GTK ===

    // Créer la fenêtre principale
    GtkWindow *window = GTK_WINDOW(gtk_window_new());
    gtk_window_set_title(window, "Chat Client");
    gtk_window_set_default_size(window, 500, 400);
    g_signal_connect(window, "close-request", G_CALLBACK(on_window_close), NULL);

    // Créer un conteneur vertical pour les éléments
    GtkBox *vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_top(GTK_WIDGET(vbox), 10);
    gtk_widget_set_margin_bottom(GTK_WIDGET(vbox), 10);
    gtk_widget_set_margin_start(GTK_WIDGET(vbox), 10);
    gtk_widget_set_margin_end(GTK_WIDGET(vbox), 10);

    // Créer la zone d'affichage des messages (TextBuffer + TextView)
    GtkTextView *text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(text_view, FALSE);  // En lecture seule
    gtk_text_view_set_cursor_visible(text_view, FALSE);
    gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD);  // Retour à la ligne
    app_state->text_buffer = gtk_text_view_get_buffer(text_view);

    // Ajouter une scrollbar à la zone de messages
    GtkScrolledWindow *scrolled = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_child(scrolled, GTK_WIDGET(text_view));
    gtk_scrolled_window_set_has_frame(scrolled, TRUE);
    gtk_widget_set_hexpand(GTK_WIDGET(scrolled), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scrolled), TRUE);

    // Créer un conteneur horizontal pour le champ de saisie et le bouton Send
    GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

    // Créer le champ de saisie
    app_state->entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_placeholder_text(app_state->entry, "Type a message...");
    gtk_widget_set_hexpand(GTK_WIDGET(app_state->entry), TRUE);

    // Ajouter un gestionnaire pour la touche Entrée
    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_press), app_state);
    gtk_widget_add_controller(GTK_WIDGET(app_state->entry), key_controller);

    // Créer le bouton "Send"
    GtkButton *send_button = GTK_BUTTON(gtk_button_new_with_label("Send"));
    g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_clicked), app_state);

    // Ajouter les widgets au conteneur horizontal
    gtk_box_append(hbox, GTK_WIDGET(app_state->entry));
    gtk_box_append(hbox, GTK_WIDGET(send_button));

    // Ajouter les conteneurs au conteneur vertical principal
    gtk_box_append(vbox, GTK_WIDGET(scrolled));
    gtk_box_append(vbox, GTK_WIDGET(hbox));

    // Ajouter le conteneur vertical à la fenêtre
    gtk_window_set_child(window, GTK_WIDGET(vbox));
    gtk_widget_show(GTK_WIDGET(window));

    // Afficher un message de bienvenue
    queue_message(app_state, "Connected to server!");

    // === Démarrer le thread de réception ===

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, receive_messages_thread, app_state) != 0) {
        perror("Failed to create thread");
        close(client_socket);
        free(app_state);
        return 1;
    }

    // === Boucle principale GTK ===

    // Boucle qui continue tant que should_stop est false
    while (1) {
        pthread_mutex_lock(&state_mutex);
        if (should_stop) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }
        pthread_mutex_unlock(&state_mutex);

        // Traiter les événements GTK
        g_main_context_iteration(NULL, TRUE);
    }

    // Attendre que le thread de réception se termine
    pthread_join(thread_id, NULL);

    // Nettoyer la mémoire
    free(app_state);

    return 0;
}
