#*******************************************************************************
#
# Makefile for TinyXml static lib (archive).
#
#*******************************************************************************

#*******************************************************************************
# Compiler
#*******************************************************************************
CXX = c++

#*******************************************************************************
# Debug flags
#*******************************************************************************
#CXXFLAGS = -g -Wall -DTIXML_USE_STL

#*******************************************************************************
# Release flags
#*******************************************************************************
CXXFLAGS = -O -Wall -DTIXML_USE_STL

#*******************************************************************************
# Archive to create
#*******************************************************************************
tinyxml.a: $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o
	$(AR) $(ARFLAGS) tinyxml.a $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o

nostl: CXXFLAGS = -O -Wall
nostl: $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o
	$(AR) $(ARFLAGS) tinyxml.a $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o
#*******************************************************************************
# Files to compile
#*******************************************************************************
$(CURDIR)/obj/tinystr.o: $(CURDIR)/tinystr.cpp $(CURDIR)/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/tinystr.cpp -o $(CURDIR)/obj/tinystr.o

$(CURDIR)/obj/tinyxml.o: $(CURDIR)/tinyxml.cpp $(CURDIR)/tinyxml.h $(CURDIR)/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/tinyxml.cpp -o $(CURDIR)/obj/tinyxml.o

$(CURDIR)/obj/tinyxmlerror.o: $(CURDIR)/tinyxmlerror.cpp $(CURDIR)/tinyxml.h $(CURDIR)/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/tinyxmlerror.cpp -o $(CURDIR)/obj/tinyxmlerror.o

$(CURDIR)/obj/tinyxmlparser.o: $(CURDIR)/tinyxmlparser.cpp $(CURDIR)/tinyxml.h $(CURDIR)/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/tinyxmlparser.cpp -o $(CURDIR)/obj/tinyxmlparser.o

#*******************************************************************************
# Cleanup
#*******************************************************************************
clean:
	@if test -f tinyxml.a; then \
		rm $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o tinyxml.a && \
		echo rm $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o tinyxml.a; \
	else \
		echo "tinyxml.a not exist, leaving"; \
	fi
