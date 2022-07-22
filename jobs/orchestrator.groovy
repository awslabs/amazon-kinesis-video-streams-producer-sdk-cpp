import jenkins.model.*

WORKSPACE_PRODUCER="samples"
// WORKSPACE_CONSUMER="consumer-java"
GIT_URL='https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git'
GIT_HASH='canary-producer-cpp'
RUNNER_JOB_NAME_PREFIX = "producer-runner"

// TODO: Set up configurability to run different parameter combinations
// Run long run canary for 12 hours
LONG_RUN_DURATION_IN_SECONDS = 12 * 60 * 60
SHORT_RUN_DURATION_IN_SECONDS = 30
COLD_STARTUP_DELAY_IN_SECONDS = 60 * 60
MIN_RETRY_DELAY_IN_SECONDS = 60
FRAGMENT_SIZE_IN_BYTES = 250000
AWS_DEFAULT_REGION = "us-west-2"

COMMON_PARAMS = [
    string(name: 'AWS_KVS_LOG_LEVEL', value: "1"),
    string(name: 'GIT_URL', value: GIT_URL),
    string(name: 'GIT_HASH', value: GIT_HASH),
    string(name: 'MIN_RETRY_DELAY_IN_SECONDS', value: MIN_RETRY_DELAY_IN_SECONDS.toString()),
]

def getJobLastBuildTimestamp(job) {
    def timestamp = 0
    def lastBuild = job.getLastBuild()      
    if (lastBuild != null) {
        timestamp = lastBuild.getTimeInMillis()
    }

    return timestamp
}

def cancelJob(jobName) {
    def job = Jenkins.instance.getItemByFullName(jobName)

    echo "Tear down ${jobName}"
    job.setDisabled(true)
    job.getBuilds()
       .findAll({ build -> build.isBuilding() })
       .each({ build -> 
            echo "Kill $build"
            build.doKill()
        })
}

def findRunners() {
    def filterClosure = { item -> item.getDisplayName().startsWith(RUNNER_JOB_NAME_PREFIX) }
    return Jenkins.instance
                    .getAllItems(Job.class)
                    .findAll(filterClosure)
}

NEXT_AVAILABLE_RUNNER = null
ACTIVE_RUNNERS = [] 

pipeline {
    agent {
        label 'producer-uw2'
    }

    options {
        disableConcurrentBuilds()
    }

    stages {
        stage('Checkout') {
            steps {
                checkout([$class: 'GitSCM', branches: [[name: GIT_HASH ]],
                          userRemoteConfigs: [[url: GIT_URL]]])
            }
        }

        stage('Update runners') {
            stages {
                stage("Find the next available runner and current active runners") {
                    steps {
                        script {
                            def runners = findRunners()
                            def nextRunner = null 
                            def oldestTimestamp = Long.MAX_VALUE

                            // find the least active runner
                            runners.each {
                                def timestamp = getJobLastBuildTimestamp(it)
                                if ((it.isDisabled() || !it.isBuilding()) && timestamp < oldestTimestamp) {
                                    nextRunner = it
                                    oldestTimestamp = timestamp
                                }
                            }

                            if (nextRunner == null) {
                                error "There's no available runner"
                            }

                            NEXT_AVAILABLE_RUNNER = nextRunner.getDisplayName()
                            echo "Found next available runner: ${NEXT_AVAILABLE_RUNNER}"

                            ACTIVE_RUNNERS = runners.findAll({ item -> item != nextRunner && (!item.isDisabled() || item.isBuilding()) })
                                                    .collect({ item -> item.getDisplayName() })
                            echo "Found current active runners: ${ACTIVE_RUNNERS}"
                        }
                    }
                }
            
                stage("Spawn new runners") {
                    steps {
                        script {
                            echo "New runner: ${NEXT_AVAILABLE_RUNNER}"
                            Jenkins.instance.getItemByFullName(NEXT_AVAILABLE_RUNNER).setDisabled(false)

                            def gitHash = sh(returnStdout: true, script: 'git rev-parse HEAD')
                            COMMON_PARAMS << string(name: 'GIT_HASH', value: gitHash)
                        }

                        // TODO: Use matrix to provide configurability in parameters
                        build(
                            job: NEXT_AVAILABLE_RUNNER,
                            parameters: COMMON_PARAMS + [
                                string(name: 'CANARY_STREAM_NAME', value: "Continuous-Longrun-01"),
                                string(name: 'CANARY_DURATION_IN_SECONDS', value: LONG_RUN_DURATION_IN_SECONDS.toString()),
                                string(name: 'PRODUCER_NODE_LABEL', value: "producer-uw2"),
                                // string(name: 'CANARY_TYPE', value: "RealtimeStatic"),
                                string(name: 'RUNNER_LABEL', value: "Longrun"),
                                // string(name: 'FRAGMENT_SIZE_IN_BYTES', value: FRAGMENT_SIZE_IN_BYTES.toString()),
                                // string(name: 'AWS_DEFAULT_REGION', value: AWS_DEFAULT_REGION),
                                string(name: 'CANARY_RUN_SCENARIO', value: "Continuous"),
                            ],
                            wait: false
                        )

                        build(
                            job: NEXT_AVAILABLE_RUNNER,
                            parameters: COMMON_PARAMS + [
                                string(name: 'CANARY_STREAM_NAME', value: "Continuous-Periodic-02"),
                                string(name: 'CANARY_DURATION_IN_SECONDS', value: SHORT_RUN_DURATION_IN_SECONDS.toString()),
                                string(name: 'PRODUCER_NODE_LABEL', value: "producer-uw2"),
                                // string(name: 'CANARY_TYPE', value: "Realtime"),
                                string(name: 'RUNNER_LABEL', value: "Periodic"),
                                // string(name: 'FRAGMENT_SIZE_IN_BYTES', value: FRAGMENT_SIZE_IN_BYTES.toString()),
                                // string(name: 'AWS_DEFAULT_REGION', value: AWS_DEFAULT_REGION),
                                string(name: 'CANARY_RUN_SCENARIO', value: "Continuous"),
                            ],
                            wait: false
                        )
                        build(
                            job: NEXT_AVAILABLE_RUNNER,
                            parameters: COMMON_PARAMS + [
                                string(name: 'CANARY_STREAM_NAME', value: "Continuous-Periodic-03"),
                                string(name: 'CANARY_DURATION_IN_SECONDS', value: SHORT_RUN_DURATION_IN_SECONDS.toString()),
                                string(name: 'PRODUCER_NODE_LABEL', value: "producer-uw2"),
                                // string(name: 'CANARY_TYPE', value: "Realtime"),
                                string(name: 'RUNNER_LABEL', value: "Periodic"),
                                // string(name: 'FRAGMENT_SIZE_IN_BYTES', value: FRAGMENT_SIZE_IN_BYTES.toString()),
                                // string(name: 'AWS_DEFAULT_REGION', value: AWS_DEFAULT_REGION),
                                string(name: 'CANARY_RUN_SCENARIO', value: "Continuous"),
                            ],
                            wait: false
                        )
                        build(
                            job: NEXT_AVAILABLE_RUNNER,
                            parameters: COMMON_PARAMS + [
                                string(name: 'CANARY_STREAM_NAME', value: "Continuous-Periodic-04"),
                                string(name: 'CANARY_DURATION_IN_SECONDS', value: SHORT_RUN_DURATION_IN_SECONDS.toString()),
                                string(name: 'PRODUCER_NODE_LABEL', value: "producer-uw2"),
                                // string(name: 'CANARY_TYPE', value: "Realtime"),
                                string(name: 'RUNNER_LABEL', value: "Periodic"),
                                // string(name: 'FRAGMENT_SIZE_IN_BYTES', value: FRAGMENT_SIZE_IN_BYTES.toString()),
                                // string(name: 'AWS_DEFAULT_REGION', value: AWS_DEFAULT_REGION),
                                string(name: 'CANARY_RUN_SCENARIO', value: "Continuous"),
                            ],
                            wait: false
                        )
                        build(
                            job: NEXT_AVAILABLE_RUNNER,
                            parameters: COMMON_PARAMS + [
                                string(name: 'CANARY_STREAM_NAME', value: "Continuous-Periodic-05"),
                                string(name: 'CANARY_DURATION_IN_SECONDS', value: SHORT_RUN_DURATION_IN_SECONDS.toString()),
                                string(name: 'PRODUCER_NODE_LABEL', value: "producer-uw2"),
                                // string(name: 'CANARY_TYPE', value: "Realtime"),
                                string(name: 'RUNNER_LABEL', value: "Periodic"),
                                // string(name: 'FRAGMENT_SIZE_IN_BYTES', value: FRAGMENT_SIZE_IN_BYTES.toString()),
                                // string(name: 'AWS_DEFAULT_REGION', value: AWS_DEFAULT_REGION),
                                string(name: 'CANARY_RUN_SCENARIO', value: "Continuous"),
                            ],
                            wait: false
                        )
                    }
                }

                stage("Tear down old runners") {
                    when {
                        expression { return ACTIVE_RUNNERS.size() > 0 }
                    }

                    steps {
                        script {
                            try {
                                sleep COLD_STARTUP_DELAY_IN_SECONDS
                            } catch(err) {
                                // rollback the newly spawned runner
                                echo "Rolling back ${NEXT_AVAILABLE_RUNNER}"
                                cancelJob(NEXT_AVAILABLE_RUNNER)
                                throw err
                            }

                            for (def runner in ACTIVE_RUNNERS) {
                                cancelJob(runner)
                            }
                        }
                    }
                }
            }
        }
    }
}
