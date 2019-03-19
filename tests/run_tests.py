#
#  This script scanes a directory full of test cases.  For each file, it opens the file
#    and parses a set of commands.  There are two types of commands:
#     @ DO  which runs a command that should succeed, and and @DO_FAIL, which runs a command that should fail.
#      If the exit code is deviates from what is expected, the test will indicate failure
#
#   Some simple text substitution is done on the command line for maintainability 
#      $PATH$ is replaced with the path to the current file
#      $FILE$ is replaced with the current file name
#      $DIR$ is replaced with the path to the test directory
#      $EXE$ is the path to the test executable
#

import os;
import sys;
import subprocess;

test_path   = './cases'
binary_path = 'IntelShaderAnalyzer.exe'

for file in os.listdir(test_path):
    
    fullpath = os.path.join(test_path, file);

    if os.path.isfile(fullpath):
        print (fullpath);

        f = open(fullpath,'r')
        lines = f.readlines();
        f.close();

        #  extract commands
        commands = [];
        expect_fail = [];
        for line in lines :
            
            tokens = line.split();

            if len(tokens) == 0:
                continue;

            # $END$ signals end of test script
            if tokens[0] == "@END":
                break;

            if tokens[0] == "@DO":
                expect_fail.append(False);
            elif tokens[0] == "@DO_FAIL":
                expect_fail.append(True);
            else:
                continue;

            tokens = tokens[1:];
        
            # do substitution
            command = '';
            for tok in tokens :
                tok = tok.replace( "$FILE$", file )
                tok = tok.replace( "$PATH$", fullpath )
                tok = tok.replace( "$DIR$", test_path )
                tok = tok.replace( "$EXE$", binary_path )
                command = command + tok + ' ';

            commands.append(command);
            
        # run the commands
        for i in range(0, len(commands)):
            command = commands[i];
            
            print('    COMMAND: ' + command);
            sys.stdout.flush();
            
            result = os.system( command );
            fail = (result != 0);

            if( result != 0 and result != 1 ):
                print( '   UNEXPECTED RETURN CODE: ' + str(result) + ' TEST FAILED!' );
                sys.exit();

            if (result == 0 and expect_fail[i]) or (result == 1 and not expect_fail[i]):
                print( 'WRONG RETURN CODE: TEST FAILED!!!!!!!' )
                sys.exit();


print( 'TEST PASSED')
