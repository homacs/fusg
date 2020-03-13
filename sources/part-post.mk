


depend: $(CDEPS)
	


# Rule for each dependency file of a c-file in this folder
$(DEPENDS_DIR)/$(PART)/src/%.d: src/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -MM -MP -MT"$(@:%.d=%.o)" -MF "$@" $<


-include $(CDEPS)






build: $(EXECUTABLE) $(STATIC_LIBRARY) $(SHARED_LIBRARY)
	


# Rule for executables
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@  $^ $(LIBRARIES)

# Rule for statically linked libraries
$(STATIC_LIBRARY): $(OBJECTS)
	$(AR) -r $@  $^ 


# Rule for each object of a c-file in this folder
$(OBJECTS_DIR)/$(PART)/src/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

