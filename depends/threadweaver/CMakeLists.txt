project(threadweaver CXX)

find_package(Qt5Core REQUIRED)

include_directories(
	${Qt5Core_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/threadweaver/
	${PROJECT_SOURCE_DIR}/src/
  )

set(ThreadWeaver_LIB_SRCS
# Internal files
	src/collection.cpp
	src/debuggingaids.cpp
	src/dependency.cpp
	src/dependencypolicy.cpp
	src/destructedstate.cpp
	src/destructedstate_p.h
	src/exception.cpp
	src/executewrapper.cpp
	src/executewrapper_p.h
	src/executor.cpp
	src/executor_p.h
	src/iddecorator.cpp
	src/inconstructionstate.cpp
	src/inconstructionstate_p.h
	src/job.cpp
	src/qobjectdecorator.cpp
	src/queue.cpp
	src/queueapi.cpp
	src/queueapi_p.h
	src/queuesignals.cpp
	src/queuestream.cpp
	src/resourcerestrictionpolicy.cpp
	src/sequence.cpp
	src/shuttingdownstate.cpp
	src/shuttingdownstate_p.h
	src/state.cpp
	src/suspendedstate.cpp
	src/suspendedstate_p.h
	src/suspendingstate.cpp
	src/suspendingstate_p.h
	src/thread.cpp
	src/threadweaver.cpp
	src/weaver.cpp
	src/weaver_p.h
	src/weaverimplstate.cpp
	src/weaverimplstate_p.h
	src/weaverinterface_p.h
	src/workinghardstate.cpp
	src/workinghardstate_p.h

# External headers
	threadweaver/collection.h
	threadweaver/debuggingaids.h
	threadweaver/dependency.h
	threadweaver/dependencypolicy.h
	threadweaver/exception.h
	threadweaver/iddecorator.h
	threadweaver/job.h
	threadweaver/jobinterface.h
	threadweaver/jobpointer.h
	threadweaver/lambda.h
	threadweaver/managedjobpointer.h
	threadweaver/qobjectdecorator.h
	threadweaver/queue.h
	threadweaver/queueing.h
	threadweaver/queueinterface.h
	threadweaver/queuepolicy.h
	threadweaver/queuesignals.h
	threadweaver/queuestream.h
	threadweaver/resourcerestrictionpolicy.h
	threadweaver/sequence.h
	threadweaver/state.h
	threadweaver/thread.h
	threadweaver/threadweaver.h
	threadweaver/threadweaver_export.h
)

add_definitions(-DTHREADWEAVER_LIBRARY)
add_definitions(-DTHREADWEAVER_STATIC)

add_library(threadweaver STATIC ${ThreadWeaver_LIB_SRCS})
target_link_libraries(threadweaver PUBLIC Qt5::Core)

