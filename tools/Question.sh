
#
# File: Question.sh
#
# Copyright 1995, by Hewlett-Packard Company
#
# The code in this file is from the book "Shell Programming
# Examples" by Bruce Blinn, published by Prentice Hall.
# This file may be copied free of charge for personal,
# non-commercial use provided that this notice appears in
# all copies of the file.  There is no warranty, either
# expressed or implied, supplied with this code.
#

Question() {
     #
     # NAME
     #    Question - ask a question
     #
     # SYNOPSIS
     #    Question question default helpmessage
     #
     # DESCRIPTION
     #    This function will print a question and return the
     #    answer entered by the user in the global variable
     #    ANSWER.  The question will be printed to the
     #    standard output.  If a default answer is supplied,
     #    it will be enclosed in square brackets and
     #    appended to the question.  The question will then
     #    be followed with a question mark and printed
     #    without a newline.
     #
     #    The default answer and the help message may be
     #    omitted, but an empty parameter (i.e., "") must
     #    be passed in their place.
     #
     #    The user may press enter without entering an
     #    answer to accept the default answer.
     #
     #    The user may enter "quit" or "q" to exit the
     #    command file.  This answer is not case sensitive.
     #
     #    The user may enter a question mark to receive a
     #    help message if one is available.  After the help
     #    message is printed, the question will be printed
     #    again.
     #
     #    The user may enter !command to cause the UNIX
     #    command to be executed.  After the command is
     #    executed, the question will be repeated.
     #
     #    The answers -x and +x cause the debugging option
     #    in the shell to be turned on and off respectively.
     #
     #    For "yes and no" questions, "yes", "y", "no", or,
     #    "n" can be entered.  This response is not case
     #    sensitive.
     #
     #    The answer will be returned exactly as the user
     #    entered it except "yes" or "no" will be returned
     #    for yes or no questions, and the default answer
     #    will be returned if the user enters a return.
     #    
     if [ $# -lt 3 ]; then
          echo "Usage: Question question" \
               "default helpmessage" 1>&2
          return
     fi
     ANSWER=             # Global variable for answer
     
     typeset _DEFAULT
     typeset _QUESTION
     typeset _HELPMSG
     
     _DEFAULT=$2         # Default answer
     _QUESTION=          # Question as it will be printed
     _HELPMSG=$3         # Text of the help message

     if [ "$_DEFAULT" = "" ]; then
          _QUESTION="$1? "
     else
          _QUESTION="$1 [$_DEFAULT]? "
     fi

     while :
     do
          if [ "`echo -n`" = "-n" ]; then
               echo "$_QUESTION\c"
          else
               echo -n "$_QUESTION"
          fi
          read ANSWER
          case `echo "$ANSWER" | tr '[A-Z]' '[a-z]'` in
               "" ) if [ "$_DEFAULT" != "" ]; then
                         ANSWER=$_DEFAULT
                         break
                    fi
                    ;;

               yes | y )
                    ANSWER=yes
                    break
                    ;;

               no | n )
                    ANSWER=no
                    break
                    ;;

               quit | q )
                    exit 1
                    ;;

               +x | -x )
                    set $ANSWER
                    ;;

               !* ) eval `expr "$ANSWER" : "!\(.*\)"`
                    ;;

               "?" )echo ""
                    if [ "$_HELPMSG" = "" ]; then
                         echo "No help available."
                    else
                         echo "$_HELPMSG"
                    fi
                    echo ""
                    ;;

               * )  break
                    ;;
          esac
     done
}
