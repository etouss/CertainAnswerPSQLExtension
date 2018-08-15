In order to compile : "make USE_PGXS=1 install"
In order to use : psql# load 'certain_answer_codd';
                  psql# load 'certain_answer_marked';
                  
                  psql# SET certain_answer_codd.certain to 0;  (default answer)
                  psql# SET certain_answer_codd.certain to 1;  (certain answer)
                  psql# SET certain_answer_codd.certain to 2;  (possible answer)
