


#  Begin_Interface_(FileServer)
#
#
#  Procedure_(Read, InWord_(handle), InWord_(FilePointer), InWord_(length),
#   				OutWord_(result), OutMem_(BufferAddress, length) )


define(`ifundef', ifdef($1,,$2))

define(`In', `define(`Mode',`in')')
define(`Out',`define(`Mode',`out')')

define(`Word_', `ifelse(Mode,`in',`InWord_($1)', `OutWord_($1)')')


define(`InWord_',  `ifundef(`SndWord0', `define(`SndWord0', $1 )') 
				   	ifundef(`SndWord1', `define(`SndWord1', $1 )') 
					ifundef(`SndWord2', `define(`SndWord2', $1 )') ' )


In Word_(handle) 
Word_(pointer)

SndW0 = SndWord0 ; SndW1 = SndWord1 ; SndW2 = SndWord2 ;
