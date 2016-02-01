CS 571 – Programming Assignment Part II, Due by Date: 11/25/2014
Assignment solution by-
Student Id: 12022254
Name: Jayanti Andhale
--------------------------------------------------------------------------------------------------------------------------------------------------
Summary:
Program: Implementation of Distance Vector routing protocol for a fixed topology
INPUT: Configuration file in .txt format
OUTPUT: 1. Prints Initial Routing Table
        2. Sends Distance vectors to neighbors
        3. Keeps receiving distance vectors from neighbors
        4. Periodically (after each timeout) sends distance vector to neighbors
        5. Prints Received distance vector
        6. Prints series of Updated Routing Tables
ASSUMPTIONS:
 1. Topology is fixed with nodes A,B,C,D,E,F,G
 2. For unreachable node: distance is MAX_DIST and next_hop is UNKNOWN_HOP
 3. Valid distance is greater than 0 and less than MAX_DIST (here 1000)
 4. In order to test the broken link; 
	Option 1. Remove nodes from config file of both sides of a link
	Option 2. Put link cost >=MAXDIST 
(Make sure to run the protocol at these nodes before at any other node)
5. Program doesn’t solve problem of count to infinity (dynamic changes)

--------------------------------------------------------------------------------------------------------------------------------------------------

Instructions to Compile and to Run the program files:
1. All compilation rules are in Make file, so run Makefile ->
	make -f Makefile 
2. Run Dist_Vect along with input config.txt file
	./Dist_Vect Config.txt
3. Press Ctr+C to exit from forever running loop
4. Run clean to clean object files created
	make –f Makefile Clean

--------------------------------------------------------------------------------------------------------------------------------------------------

