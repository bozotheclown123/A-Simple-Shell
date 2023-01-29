/* Name: Muzzamal Saleem
   UID: 116270786 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "command.h"
#include "executor.h"

static int execute_aux(struct tree *t, int input_fd, int output_fd);

int execute(struct tree *t) {

   return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
}

static int execute_aux(struct tree *t, int input_fd, int output_fd) { 
   pid_t process_id;
   int status;
   
   if (t->conjunction == NONE) {
	 
      if (strcmp(t->argv[0], "cd") == 0) {	    
	      if (t -> argv[1] != NULL){
            if (chdir(t->argv[1]) == -1) {
               perror(t->argv[1]);
            }
         } else {
            chdir(getenv("HOME"));
         }
      }
      
      else if (strcmp(t->argv[0], "exit") == 0) {
	      exit(0); 
      }
      else {
         process_id = fork();     
         if (process_id == -1) {
            perror("fork");
         }
         else if (process_id > 0){
            wait(&status);
            return status;
         }
         else if (process_id == 0){

            if (t->input != NULL) {
               input_fd = open(t->input, O_RDONLY);
		         if (input_fd == -1) {
		            err(EX_OSERR, "NONE: An error occured with opening input file.\n");     
               }
		         if (dup2(input_fd, STDIN_FILENO) == -1) {
		            err(EX_OSERR, "NONE: A dup2 (STDIN) error occured.\n");
               }	     
		         close(input_fd);
            }      	       

	         if (t->output != NULL) {
               output_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		         if (output_fd == -1) {
		            err(EX_OSERR, "NONE: An error occured with opening output file.\n");
               }		     
		         if (dup2(output_fd, STDOUT_FILENO) == -1) {
		            err(EX_OSERR, "NONE: A dup2 (STDOUT) error occured.\n");
               }		     
		         close(output_fd);		  
            }
	      
	         if (execvp(t->argv[0], t->argv) == -1) { 
          /* If execvp doesn't work, then the execvp program will not take over 
          and will print the error message and exit out */
	            fprintf(stderr, "Failed to execute %s\n", t->argv[0]);    
	            exit(1); 	    
            }
         }
      }

   } else if (t->conjunction == AND) {

      if (t->input != NULL) {
         input_fd = open(t->input, O_RDONLY);
	      if (input_fd == -1) {
	         err(EX_OSERR, "AND: An error occured with opening input file.\n");
         }
      }

      if (t->output != NULL) {
         output_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	      if (output_fd == -1) {
	         err(EX_OSERR, "AND: An error occured with opening output file.\n");
         }
      }

      if (execute_aux(t->left, input_fd, output_fd) == 0) {
         return execute_aux(t->right, input_fd, output_fd);
      } else {
         return execute_aux(t->left, input_fd, output_fd);
      }

   } else if (t->conjunction == PIPE) {	 
      int pipe_fd[2];	 	 
	 
      if (t->left->output) {
	      printf("Ambiguous output redirect.\n");	   
      } else {
	      if (t->right->input) {
	         printf("Ambiguous input redirect.\n");
         } else {	       

	            if (t->input != NULL) {
                  input_fd = open(t->input, O_RDONLY);
	               if (input_fd  == -1) {
		               err(EX_OSERR, "PIPE: An error occured with opening input file.\n");
                  }
               }
	         if (t->output != NULL) {
               output_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	            if (output_fd == -1) {
		            err(EX_OSERR, "PIPE: An error occured with opening output file.\n");
               }
            }       
	   
	         if (pipe(pipe_fd) == -1) {
	            err(EX_OSERR, "Error: pipe error\n");
            }
            process_id = fork();	       
	         if (process_id == -1) {
	            perror("fork");
            }
	    
	         if (process_id == 0) {
	            close(pipe_fd[0]);
	            if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
		            err(EX_OSERR, "Error: dup2 error\n");
               }
	            execute_aux(t->left, input_fd, pipe_fd[1]);		  
	            close(pipe_fd[1]);
	            exit(0);
            }
	    
	         if (process_id != 0) {	  
	            close(pipe_fd[1]);		  
	            if (dup2(pipe_fd[0], STDIN_FILENO) == -1) {
		            err(EX_OSERR, "Error: dup2 error\n");
               }
	            execute_aux(t->right, pipe_fd[0], output_fd);	  
	            close(pipe_fd[0]);
	            wait(NULL); 
            }
         }	 	 
      }

   }  else if (t->conjunction == SUBSHELL) {	 

         if (t->input != NULL) {
	         if ((input_fd = open(t->input, O_RDONLY)) < 0) {
	            err(EX_OSERR, "SUBSHELL: An error occured with opening input file.\n");
            }
         }
         if (t->output != NULL) {
	         if ((output_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0) {
	            err(EX_OSERR, "SUBSHELL: An error occured with opening output file.\n");
            }
         }	 
         process_id = fork();
         if (process_id  == -1) {
	         perror("fork");
         } 
         else { 
	         if (process_id != 0) {
	            wait(NULL);
            }
         /*  Ignore the t->right subtree. */
	         if (process_id == 0) {
	            execute_aux(t->left, input_fd, output_fd);
	            exit(0);
            }
         }
   } 
   return 0;
} 
            
