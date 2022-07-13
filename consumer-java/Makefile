test: build

install-system-dependencies:
	@echo "Installing system dependencies"

build: install-system-dependencies
	@echo "Build java parser based consumer"
	mvn package
	# Create a temporary filename in /tmp directory
	touch tmp_jar
	# Create classpath string of dependencies from the local repository to a file
	mvn -Dmdep.outputFile=tmp_jar dependency:build-classpath