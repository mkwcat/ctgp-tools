CW_PATH=../CW
CC=$CW_PATH/mwcceppc
CCFILES="Main Cheats String Cache"
CFLAGS="-I- -i . -i ../include -i ../include/k_stdlib -Cpp_exceptions off -proc gekko -enc SJIS -enum int -fp hard -inline auto,smart -O4,p -opt space -func_align 4 -rostr -sdata 0 -sdata2 0 -use_lmw_stmw on"

MakeDirectoryOrFail()
{
	mkdir -p $1
	if [ ! -d $1 ]; then
		echo "Failed to create the directory '$1', exiting !" 1>&2
		exit 1
	fi
}

REGIONS="E P J"
for region in $REGIONS
do
	GAME_ID="RMC$region"

	printf "********************** START BUILD $GAME_ID *********************\n"

	sed -i '$d' Main.hh
	echo "#define $GAME_ID" >> Main.hh

	OBJECTS=""
	for ccFile in $CCFILES
	do
		echo "Compiling $ccFile.cc..."
		
		OBJECT_DIRECTORY="../object/$GAME_ID"
		MakeDirectoryOrFail $OBJECT_DIRECTORY
		
		OBJECTS="$OBJECTS $OBJECT_DIRECTORY/$ccFile.o"
		$CC $CFLAGS -c -o "$OBJECT_DIRECTORY/$ccFile.o" "$ccFile.cc"
	done
	printf "\n"

	EXTERNALS_DIRECTORY="../externals"
	MakeDirectoryOrFail $EXTERNALS_DIRECTORY
	CODE_DIRECTORY="../code/$GAME_ID"
	MakeDirectoryOrFail $CODE_DIRECTORY

	echo Linking...
	../Kamek/Kamek.exe $OBJECTS -static=0x800014B0 -externals="$EXTERNALS_DIRECTORY/$GAME_ID.txt" -output-code="$CODE_DIRECTORY/payload.bin"

	printf "********************** STOP  BUILD $GAME_ID *********************\n"
done