#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

using namespace std;

#define MAX_QUESTIONS 100
#define MAX_STUDENTS 100
#define MAX_GRADE 100

struct message {
    long mtype;
    char mtext[MAX_QUESTIONS];
};

struct answer {
    long mtype;
    int mvalue;
};

int msgqid[MAX_STUDENTS], ansqid[MAX_STUDENTS];

void childProcess(int pid, int n_students, int n_questions) {
    usleep(350);
    message msg;
    answer ans;
    // receiving message for each question and sending answer to that question
    for (int j = 0; j < n_questions; j++) {
        if (msgrcv(msgqid[pid], &msg, sizeof(msg), 1, 0) == -1) {
            // printf("Err RCV Pid : %d \n", pid);
            perror("msgrcv");
            exit(1);
        }
        printf("P%d <== %s\n", pid, msg.mtext);
        srand(pid);
        int ans_value = rand() % 4; // Random answer (0-3)
        ans.mtype = pid + 1;
        ans.mvalue = ans_value;
        printf("P%d : Q%d ans = %d send\n", pid, j, ans.mvalue);
        if (msgsnd(ansqid[pid], &ans, sizeof(ans), 0) == -1) {
            // printf("Error snd Pid : %d \n", pid);
            // printf("Error snd Pid : %d , Q %d\n", pid, j);
            perror("msgsnd");
            exit(1);
        }
    }
    exit(0);
}

void parentProcess(int n_students, int n_questions) {
    message msg;
    answer ans;
    // Send exam questions to students
    for (int i = 0; i < n_students; i++) {
        // send questions one by one
        for (int j = 0; j < n_questions; j++) {
            msg.mtype = 1;
            sprintf(msg.mtext, "Question %d", j+1);
            if (msgsnd(msgqid[i], &msg, sizeof(msg), 0) == -1) {
                // printf("ERR SND Parent\n");                
                perror("msgsnd");
                exit(1);
            }
            printf("Parent : Stud %d, Q%d  question = %s\n", i, j, msg.mtext);
            usleep(350);
        }
    }
     // Array is created to save the real randomly generated answers
    int realAns[n_questions];
    for (int i = 0; i < n_questions; i++)
    {
        realAns[i] = rand() % 4; // Random Correct Answer (0-3)
    }

    //score to save scores of all students
    int score[n_students] = {0};
    for (int i = 0; i < n_students; i++) {
        for (int j = 0; j < n_questions; j++) {
            if (msgrcv(ansqid[i], &ans, sizeof(ans), i + 1, 0) == -1) {
                perror("msgrcv");
                exit(1);
            }
            if (ans.mvalue == realAns[j]) {
                score[i]++;
            }
            printf("Parent <-- stud%d, Q%d, ans = %d\n", i, j, ans.mvalue);
            // cout << "Received Answer P" << i <<  " Q" << j << " : " << ans.mvalue << endl;
        }
    }
    // cout << endl << endl;
    printf("-----------------------\n");
    printf("| Questions | Answers |\n");
    printf("|---------------------|\n");
    for(int i = 0; i < n_questions; i++) {
        printf("|      Q%2d  |       %d |\n", i, realAns[i]);
    }
    printf("-----------------------\n");

    int totalscore = 0;

    printf("\nStudents Grades:\n");
    printf("|----------------------------------------|\n");
    printf("| Student No | Correct Questions | Grade |\n");
    printf("|----------------------------------------|\n");
    for (int i = 0; i < n_students; i++)
    {
        printf("|         %2d |           %3d/%3d |     %d |\n", i, score[i], n_questions, (score[i]*10)/n_questions);
        // cout <<"The score of the student " << i << " is " << score[i] << endl;
        totalscore += (score[i]*10)/n_questions;
    }
    printf("|----------------------------------------|\n\n");

    // float avgscore = (float)totalscore / n_students;
    // cout << endl;
    printf("Average grade = %d\n", (totalscore)/n_students);
    // cout << "The overall average score is " << avgscore << endl << endl;
}


int main() {
    int n_students, n_questions;
    srand(time(NULL));
    message msg;
    answer ans;
    int student_id, grade, total_grade = 0, max_grade = 0, min_grade = MAX_GRADE;
    // int msgqid[MAX_STUDENTS], ansqid[MAX_STUDENTS];
    int student_grade[MAX_STUDENTS] = {0};
    pid_t pid[MAX_STUDENTS], childpid;
    
    cout << "Enter the number of students taking the exam: ";
    cin >> n_students;
    cout << "Enter the number of questions on the exam: ";
    cin >> n_questions;
    
    // Create message queues
    for (int i = 0; i < n_students; i++) {
        msgqid[i] = msgget(IPC_PRIVATE, 0666);
        if (msgqid[i] == -1) {
            perror("msgget");
            exit(1);
        }
        ansqid[i] = msgget(IPC_PRIVATE, 0666);
        if (ansqid[i] == -1) {
            perror("msgget");
            exit(1);
        }
    }
    
    // Create child processes
    for (int i = 0; i < n_students; i++) {
        pid[i] = fork();
        if (pid[i] == -1) {
            perror("fork");
            exit(1);
        }
        else if (pid[i] == 0) { // Child process
            childProcess(i, n_students, n_questions);
        }
    }
    
    parentProcess(n_students, n_questions);

    for (int i = 0; i < n_students; i++) {
        msgctl(msgqid[i], IPC_RMID, NULL);
        msgctl(ansqid[i], IPC_RMID, NULL);
    }
    
}
