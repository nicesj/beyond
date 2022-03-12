#!/bin/bash

shopt -o pipefail

# NOTE:
# If the line coverage under the ${COVERAGE_LIMIT}%,
# it will be marked with RED color
COVERAGE_LIMIT=50

echo Executing Test

if [ $# -ne 2 ]; then
	BUILDDIR=$1

	echo "Build Directory is specified: ${BUILDDIR}"
fi

function test_x() {
	REPORT_FUNC=""
	REPORT_FILE=""

	TARGET=${1}
	LIST_OF_FILES=`ls ${TARGET}/src/*.cc 2>/dev/null`
	if [ $? -ne 0 ]; then
		echo "Skip ${TARGET}, implementation files are not discovered"
		return 0
	fi

	IFS=$'\n\r'
	for I in $LIST_OF_FILES
	do
		# CHECKME
		# several .gcno files are created.
		# It is necessary to understand why several .gcno files are generated even the script runs only a single test application.
		#
		# NOTE
		# at least now, just cut the first found file using "head" command
		FILENAME=`basename $I`
		GCNO=`find . -name "*$FILENAME.gcno" -print | grep -v "/test/" | grep -v "/ut/" | head -n 1`
		if [ $? -ne 0 ]; then
			echo "Unable to find a coverage file $I.gcno"
			return -1
		elif [ x"$GCNO" != x"" -a -f "$GCNO" ]; then
			# NOTE
			# Prevent from verbose logging, let's go forward silently.
			# echo "\"$GCNO\" file found"
			pushd subprojects > /dev/null
			REPORT=`gcov -a -f ../$GCNO` # -m : unmangling
			if [ $? -ne 0 ]; then
				echo "gcov returns error with $GCNO"
				echo "Skip this error and go ahead"
				continue
				#popd > /dev/null
				#return -1
			fi
			popd > /dev/null
		elif [ x"$GCNO" == x"" ]; then
			echo "Please check the coverage option for building project"
			echo "Assuming that the coverage option is disabled"
			return 0
		else
			echo "$GCNO is not accessible"
			return -1
		fi

		for LINE in $REPORT
		do
			TOKEN=`echo $LINE | awk '{print $1}'`

			if [ x"$TOKEN" == x"Function" ]; then
				TMP=`echo $LINE | awk '{print $2}'`
				FUNC=`echo $TMP | c++filt`
			elif [ x"$TOKEN" == x"File" ]; then
				FILE=`echo $LINE | awk '{print $2}'`
			elif [ x"$TOKEN" == x"Lines" ]; then
				COV=`echo $LINE | awk '{print $2}' | awk -F: '{print $2}' | awk -F% '{print $1}'`
				TEST=`echo "$COV < $COVERAGE_LIMIT" | bc -l`
				if [ $TEST -eq 1 ]; then
					WARN_ON="\e[31m"
					WARN_OFF="\e[0m"
				fi

				if [ x"$FUNC" != x"" ]; then
					FILTER=`echo $FUNC | grep -v "^'std::*" | grep -v "^'__gnu_cxx::*" | grep -v "^'grpc::*"`
					if [ x"$FILTER" != x"" ]; then
						REPORT_ITEM="$WARN_ON $FUNC $COV $WARN_OFF\n"
						REPORT_FUNC="$REPORT_FUNC$REPORT_ITEM"
					fi
					FUNC=""
				elif [ x"$FILE" != x"" ]; then
					FILTER=`echo $FILE | grep "/usr/"`
					if [ x"$FILTER" == x"" ]; then
						REPORT_ITEM="\e[32m$FILE $WARN_ON $COV $WARN_OFF\n"
						REPORT_FILE="$REPORT_FILE$REPORT_ITEM"
					fi
					FILE=""
					# TODO:
					# Should break the build script
				fi

				WARN_ON="\e[0m"
				WARN_OFF="\e[0m"
				COV=""
			fi
		done
	done

	echo -ne $REPORT_FILE
	echo -ne $REPORT_FUNC
	return 0
}

PROJECT_LIST=`ls -d subprojects/lib*`
EXCLUDE_LIST="libbeyond-mock"

for TARGET in $PROJECT_LIST; do
	EXCLUDED=0
	for EX_TARGET in $EXCLUDE_LIST; do
		FILTER=`basename $TARGET`
		if [ x"${EX_TARGET}" == x"${FILTER}" ]; then
			EXCLUDED=1
			break
		fi
	done

	if [ ${EXCLUDED} -eq 0 ]; then
		echo "Testing target project ${TARGET}"
		test_x ${TARGET}
		RET=$?
		if [ $RET -ne 0 ]; then
			break
		fi
	fi
done

exit $RET
