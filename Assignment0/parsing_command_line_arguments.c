#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
  int a = 0, b = 0;
  int opt;

  // Process the command line arguments
  // -a and -b are options ('opt'). 
  // Options can have their own arguments ('optarg', i.e. 2 or 4).

  while ((opt = getopt (argc, argv, "a:b:")) != -1)
    {
      switch (opt)
        {
        case 'a':
          a = atoi (optarg);
          break;
        case 'b':
          b = atoi (optarg);
          break;
        default:
          fprintf (stderr, "Please, retry to run with this format: %s -aA -bB\n", argv[0]);
          exit (EXIT_FAILURE);
        }
    }

  // Check that both arguments were provided
  if (a == 0 || b == 0)
    {
      fprintf (stderr, "Please, retry to run with this format: %s -aA -bB\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  // Print the result
  printf ("A is %d and B is %d\n", a, b);
  return 0;
}
