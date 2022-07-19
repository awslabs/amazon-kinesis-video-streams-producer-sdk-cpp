import jenkins.model.*
import org.jenkinsci.plugins.workflow.steps.FlowInterruptedException

START_TIMESTAMP = new Date().getTime()
HAS_ERROR = false

RUNNING_NODES=0

CREDENTIALS = [
    [
        $class: 'AmazonWebServicesCredentialsBinding', 
        accessKeyVariable: 'AWS_ACCESS_KEY_ID',
        credentialsId: 'CANARY_CREDENTIALS',
        secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
    ]
]

def buildProducer() {
  sh  """ 
    mkdir -p build &&
    cd build && 
    cmake .. -DBUILD_GSTREAMER_PLUGIN=ON && 
    make -j4
  """
}

// def buildConsumer(envs) {
//   withEnv(envs) {
//     sh '''
//         PATH="$JAVA_HOME/bin:$PATH"
//         export PATH="$M2_HOME/bin:$PATH"
//         cd $WORKSPACE/consumer-java
//         make -j4
//     '''
//   }
// }
  
def withRunnerWrapper(envs, fn) {
    withEnv(envs) {
        withCredentials(CREDENTIALS) {
            try {
                fn()
            } catch (FlowInterruptedException err) {
                echo 'Aborted due to cancellation'
                throw err
            } catch (err) {
                HAS_ERROR = true
                // Ignore errors so that we can auto recover by retrying
                unstable err.toString()
            }
        }
    }
}

def runClient(isProducer, params) {
    // def consumerEnvs = [        
    //     'JAVA_HOME': "/opt/jdk-13.0.1",
    //     'M2_HOME': "/opt/apache-maven-3.6.3"
    // ].collect({k,v -> "${k}=${v}" })

    // TODO: get the branch and version from orchestrator
    if (params.FIRST_ITERATION) {

        // TODO: Move to deletDir(). deleteDir() causes an exception right now 
        sh """
            cd $WORKSPACE
            rm -rf *
        """
    }
    
    //def consumerStartUpDelay = 45
    echo "NODE_NAME = ${env.NODE_NAME}"
    checkout([
        scm: [
            $class: 'GitSCM', 
            branches: [[name: params.GIT_HASH]],
            userRemoteConfigs: [[url: params.GIT_URL]]
        ]
    ])

    RUNNING_NODES++
    echo "Number of running nodes: ${RUNNING_NODES}"
    if(isProducer) {
        buildProducer()
    }
    // else {
    //     // This is to make sure that the consumer does not make RUNNING_NODES
    //     // zero before producer build starts. Should handle this in a better
    //     // way
    //     sleep consumerStartUpDelay
    //     buildConsumer(consumerEnvs)
    // }

    RUNNING_NODES--
    echo "Number of running nodes after build: ${RUNNING_NODES}"
    waitUntil {
        RUNNING_NODES == 0
    }
    
    echo "Done waiting in NODE_NAME = ${env.NODE_NAME}"

    def scripts_dir = "$WORKSPACE/samples"
    def endpoint = "${scripts_dir}/iot-credential-provider.txt"
    def core_cert_file = "${scripts_dir}/p${env.NODE_NAME}_certificate.pem"
    def private_key_file = "${scripts_dir}/p${env.NODE_NAME}_private.key"
    def role_alias = "p${env.NODE_NAME}_role_alias"
    def thing_name = "p${env.NODE_NAME}_thing"

    def envs = [
        // 'JAVA_HOME': "/opt/jdk-13.0.1",
        'M2_HOME': "/opt/apache-maven-3.6.3",
        'AWS_KVS_LOG_LEVEL': params.AWS_KVS_LOG_LEVEL,
        // 'CANARY_STREAM_NAME_PRE': "${env.JOB_NAME}",
        'CANARY_STREAM_NAME' : "${env.JOB_NAME}" + params.CANARY_STREAM_NAME,
        'CANARY_LABEL': params.RUNNER_LABEL,
        'CANARY_TYPE': params.CANARY_TYPE,
        'FRAGMENT_SIZE_IN_BYTES' : params.FRAGMENT_SIZE_IN_BYTES,
        'CANARY_DURATION_IN_SECONDS': params.CANARY_DURATION_IN_SECONDS,
        'AWS_DEFAULT_REGION': params.AWS_DEFAULT_REGION,
        'CANARY_RUN_SCENARIO': params.CANARY_RUN_SCENARIO,
        // 'TRACK_TYPE': params.TRACK_TYPE,
    ].collect({k,v -> "${k}=${v}" })

    // if(!isProducer) {
    //     // Run consumer
    //     withRunnerWrapper(envs) {
    //         sh '''
    //             cd $WORKSPACE/consumer-java
    //             java -classpath target/aws-kinesisvideo-producer-sdk-canary-consumer-1.0-SNAPSHOT.jar:$(cat tmp_jar) -Daws.accessKeyId=${AWS_ACCESS_KEY_ID} -Daws.secretKey=${AWS_SECRET_ACCESS_KEY} com.amazon.kinesis.video.canary.consumer.ProducerSdkCanaryConsumer
    //         '''
    //     }
    // }
    // else {
        withRunnerWrapper(envs) {
            sh """
                echo "Running producer"
                ls ./samples
                cd ./build && 
                ./Canary
            """
        }
    //}
}

pipeline {
    agent {
        label params.PRODUCER_NODE_LABEL
    }

    parameters {
        string(name: 'CANARY_STREAM_NAME')
        choice(name: 'AWS_KVS_LOG_LEVEL', choices: ["1", "2", "3", "4", "5"])
        string(name: 'PRODUCER_NODE_LABEL')
        //string(name: 'CONSUMER_NODE_LABEL')
        string(name: 'GIT_URL')
        string(name: 'GIT_HASH')
        string(name: 'CANARY_TYPE')
        string(name: 'FRAGMENT_SIZE_IN_BYTES')
        string(name: 'RUNNER_LABEL')
        string(name: 'CANARY_DURATION_IN_SECONDS')
        string(name: 'MIN_RETRY_DELAY_IN_SECONDS')
        string(name: 'AWS_DEFAULT_REGION')
        string(name: 'CANARY_RUN_SCENARIO')
        // string(name: 'TRACK_TYPE')
        booleanParam(name: 'FIRST_ITERATION', defaultValue: true)
        // booleanParam(name: 'USE_IOT')
    }

    stages {
        stage('Echo params') {
            steps {
                echo params.toString()
            }
        }
        stage('Run') {
            failFast true
            parallel {
                stage('Producer') {
                    steps {
                        script {
                            runClient(true, params)
                        }
                    }
                }
                // stage('Consumer') {
                //     agent {
                //         label params.CONSUMER_NODE_LABEL
                //     }
                //     steps {
                //         script {

                //             // Only run consumer if it is not intermittent scenario
                //             if(params.CANARY_RUN_SCENARIO == "Continuous") {
                //                     runClient(false, params)
                //             }
                //         }
                //     }
                // }
            }
        }

        // In case of failures, we should add some delays so that we don't get into a tight loop of retrying
        stage('Throttling Retry') {
            when {
                equals expected: true, actual: HAS_ERROR
            }

            steps {
                sleep Math.max(0, params.MIN_RETRY_DELAY_IN_SECONDS.toInteger() - currentBuild.duration.intdiv(1000))
            }
        }
        
        stage('Reschedule') {
            steps {
                // TODO: Maybe there's a better way to write this instead of duplicating it
                build(
                    job: env.JOB_NAME,
                            parameters: [
                                string(name: 'CANARY_STREAM_NAME', value: params.CANARY_STREAM_NAME),
                                string(name: 'AWS_KVS_LOG_LEVEL', value: params.AWS_KVS_LOG_LEVEL),
                                string(name: 'PRODUCER_NODE_LABEL', value: params.PRODUCER_NODE_LABEL),
                                string(name: 'GIT_URL', value: params.GIT_URL),
                                string(name: 'GIT_HASH', value: params.GIT_HASH),
                                string(name: 'CANARY_TYPE', value: params.CANARY_TYPE),
                                string(name: 'FRAGMENT_SIZE_IN_BYTES', value: params.FRAGMENT_SIZE_IN_BYTES),
                                string(name: 'CANARY_DURATION_IN_SECONDS', value: params.CANARY_DURATION_IN_SECONDS),
                                string(name: 'MIN_RETRY_DELAY_IN_SECONDS', value: params.MIN_RETRY_DELAY_IN_SECONDS),
                                string(name: 'AWS_DEFAULT_REGION', value: params.AWS_DEFAULT_REGION),
                                string(name: 'RUNNER_LABEL', value: params.RUNNER_LABEL),
                                string(name: 'CANARY_RUN_SCENARIO', value: params.CANARY_RUN_SCENARIO),
                                booleanParam(name: 'FIRST_ITERATION', value: false)
                            ],
                    wait: false
                )
            }
        }
    }
}
