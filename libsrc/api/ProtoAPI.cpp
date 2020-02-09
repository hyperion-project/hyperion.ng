// project includes
#include <api/ProtoAPI.h>

using namespace hyperion;

ProtoAPI::ProtoAPI(QString peerAddress, Logger *log, const bool &localConnection, QObject *parent)
    : API(log, localConnection, parent)
{
    //_peerAddress = peerAddress;
    //Q_INIT_RESOURCE(JSONRPC_schemas);
}

void ProtoAPI::initialize(void)
{
    // init API, REQUIRED!
    API::init();
    // REMOVE when jsonCB is migrated
    //handleInstanceSwitch(0);

    // setup auth interface
    //connect(this, &API::onPendingTokenRequest, this, &ProtoAPI::newPendingTokenRequest);
    //connect(this, &API::onTokenResponse, this, &ProtoAPI::handleTokenResponse);

    // listen for killed instances
    //connect(_instanceManager, &HyperionIManager::instanceStateChanged, this, &ProtoAPI::handleInstanceStateChange);

    // pipe callbacks from subscriptions to parent
    //connect(_jsonCB, &JsonCB::newCallback, this, &ProtoAPI::callbackMessage);

    // notify hyperion about a jsonMessageForward
    //connect(this, &ProtoAPI::forwardJsonMessage, _hyperion, &Hyperion::forwardJsonMessage);
}