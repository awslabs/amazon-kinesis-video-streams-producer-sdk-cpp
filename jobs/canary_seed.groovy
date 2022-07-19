import java.lang.reflect.*;
import jenkins.model.*;
import org.jenkinsci.plugins.scriptsecurity.scripts.*;
import org.jenkinsci.plugins.scriptsecurity.sandbox.whitelists.*;

WORKSPACE="."
GROOVY_SCRIPT_DIR="$WORKSPACE/jobs"
DAYS_TO_KEEP_LOGS=2
NUMBER_OF_LOGS=1
MAX_EXECUTION_PER_NODE=1
NAMESPACE="producer"

void approveSignatures(ArrayList<String> signatures) {
    scriptApproval = ScriptApproval.get()
    alreadyApproved = new HashSet<>(Arrays.asList(scriptApproval.getApprovedSignatures()))
    signatures.each {
      if (!alreadyApproved.contains(it)) {
        scriptApproval.approveSignature(it)
      }
    }
}

approveSignatures([
    "method hudson.model.ItemGroup getAllItems java.lang.Class",
    "method hudson.model.Job getBuilds",
    "method hudson.model.Job getLastBuild",
    "method hudson.model.Job isBuilding",
    "method hudson.model.Run getTimeInMillis",
    "method hudson.model.Run isBuilding",
    "method jenkins.model.Jenkins getItemByFullName java.lang.String",
    "method jenkins.model.ParameterizedJobMixIn\$ParameterizedJob isDisabled",
    "method jenkins.model.ParameterizedJobMixIn\$ParameterizedJob setDisabled boolean",
    "method org.jenkinsci.plugins.workflow.job.WorkflowRun doKill",
    "staticMethod jenkins.model.Jenkins getInstance",
    "staticField java.lang.Long MAX_VALUE"
])

pipelineJob("${NAMESPACE}-orchestrator") {
	description('Run producer')
	logRotator {
		daysToKeep(DAYS_TO_KEEP_LOGS)
		numToKeep(NUMBER_OF_LOGS)
	}
    throttleConcurrentBuilds {
        maxPerNode(MAX_EXECUTION_PER_NODE)
    }
    
	definition {
        cps {
        	script(readFileFromWorkspace("${GROOVY_SCRIPT_DIR}/orchestrator.groovy"))
            sandbox()
        }
    }
}

pipelineJob("${NAMESPACE}-runner-0") {
	description('Run producer')
	logRotator {
		daysToKeep(DAYS_TO_KEEP_LOGS)
		numToKeep(NUMBER_OF_LOGS)
	}
	definition {
        cps {
        	script(readFileFromWorkspace("${GROOVY_SCRIPT_DIR}/runner.groovy"))
            sandbox()
        }
    }
}

pipelineJob("${NAMESPACE}-runner-1") {
    description('Run producer')
    logRotator {
        daysToKeep(DAYS_TO_KEEP_LOGS)
        numToKeep(NUMBER_OF_LOGS)
    }
    definition {
        cps {
            script(readFileFromWorkspace("${GROOVY_SCRIPT_DIR}/runner.groovy"))
            sandbox()
        }
    }
}

queue("${NAMESPACE}-orchestrator")
