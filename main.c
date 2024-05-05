#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


#define NTHREADS 4
#define ROWS 100
#define COLUMNS 50

/*
First process (Child process): Creates and writes a file 'data.txt'
Second process (Parent process): Creates threads
                Each thread: - reads ROWS/NTHREADS (100/4=25) line from the file.
                             - sums up the 50 numbers of each line (25 lines each thread * 50 numbers).
                             - adds the result of the local sum up to the global variable 'global_sum'.
                             - displays how many lines each thread read.
                Then the process displays the 'global_sum'.
*/

int fd;
int global_sum = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 

void sighandler(int signum){

    printf("Signal caught: %d. Do you want to exit the program? [1: Yes, 0: No]: ", signum);
    int n; // Variable to store the number.
    do{
        if (scanf("%d", &n) != 1){
            printf("Error reading the answer. Exiting the program...\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
        if (n == 1 || n == 0){
            break;
        }
        printf("[1: Yes, 0: No]: ");
    } while(1);

    if (n == 1){
        close(fd);
        exit(EXIT_FAILURE);
    } else {
        return;
    }
    
}

void* thread_func(void* args){

    int index = * (int*) args; // Which thread is executing the routine.
    int number_read;
    int local_sum = 0;
    int starting_position = index * (COLUMNS * (ROWS/NTHREADS)); // Calculating the position where each thread should start reading.
    int lines; // 4 threads have to read a total of 100 lines. So each thread must read 100/4 = 25 lines.

    pthread_mutex_lock(&lock); 
    int l = lseek(fd, starting_position * sizeof(number_read), SEEK_SET);
  
    if (l < 0) {
        perror("Error using lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // printf("Lseek moved the file pointer for thread %d to the position: %d.\n", index + 1, l); // Debugging purposes.
    
    for(lines=0; lines<COLUMNS * (ROWS/NTHREADS); lines++){
        if(read(fd, &number_read, sizeof(number_read)) < 0){
            perror("Error reading the file");
            close(fd);
            exit(EXIT_FAILURE);
        } else {
            local_sum += number_read;
        }
       
    }

    // printf("Local sum from thread %d is: %d.\t", index+1, local_sum); // Debugging purposes.
    printf("Thread %d read %d lines.\n", index + 1, lines / COLUMNS);
    
    global_sum += local_sum; 

    pthread_mutex_unlock(&lock); 
    pthread_exit(NULL);

}

int main(){

    if (signal(SIGINT, sighandler) == SIG_ERR){
        perror("Error catching the signal");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGTERM, sighandler) == SIG_ERR){
        perror("Error catching the signal");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fd = open("data.txt", O_RDWR  | O_CREAT | O_TRUNC, 0644);

    if (fd < 0){
            perror("Error creating-opening the file");
            close(fd);
            exit(EXIT_FAILURE);
        }

    int pid = fork();

    if (pid < 0){
        perror("Error creating a process");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    /*
    Inside the child process, we will create the file with the random generated numbers.
    */    
    if (pid == 0){
        printf("Inside the child process right now with PID: %d. Creating the file and generating the numbers...\n\n", getpid());  
        // sleep(15); // Testing the signal handler function.
        srand(time(NULL));

        int actual_sum = 0; // Debugging purposes.

        for (int i=0; i<ROWS; i++){
            for (int y=0; y<COLUMNS; y++){
                int generated_number = rand() % 101; // Random number from 0-100.
                actual_sum += generated_number;
                if (write(fd, &generated_number, sizeof(generated_number)) < 0){
                    perror("Error writing to file");
                    exit(EXIT_FAILURE);
                }
            }
    
        }   

        printf("Actual sum (Calculated from the child process) is: %d\n\n\n", actual_sum);
        close(fd);
        
    /*
    Inside the parent process, we will create the threads that sum up the numbers.
    */
    } else {
        
        // Waiting for the child process.
        if (wait(NULL) < 0){
            perror("Error waiting for the child process");
            close(fd);
            exit(EXIT_FAILURE);
        } 

        printf("Inside the parent process right now with PID: %d. Child process was completed. Creating the threads...\n\n", getpid());
        sleep(15); // Testing the signal handler function.
        pthread_t thread[NTHREADS];
        int count[NTHREADS];

        for(int i=0; i<NTHREADS; i++){
            count[i] = i; // Number of the current thread created.
            if (pthread_create(&thread[i], NULL, &thread_func, &count[i]) != 0){
                perror("Error creating a thread");
                close(fd);
                exit(EXIT_FAILURE);
            } else {
               // printf("Creating thread: %d\n", i+1); // Debugging purposes.
            }
        }

        for(int i=0; i<NTHREADS; i++){
            
            if (pthread_join(thread[i], NULL) != 0){
                perror("Error to join thread.");
                close(fd);
                exit(EXIT_FAILURE);
            } else {
               // printf("Closing thread: %d\n", i+1); // Debugging purposes.
            }
            
        }

        printf("All threads have finished their work.\n\n");
        printf("The total sum of the file (Calculated from the parent process) is: %d\n", global_sum);  
        pthread_mutex_destroy(&lock); 
        close(fd); 
        
        return 0;   
    }

}
