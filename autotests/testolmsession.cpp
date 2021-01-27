#include "olm/session.h"

using namespace Quotient;

std::pair<std::unique_ptr<QOlmSession>, std::unique_ptr<QOlmSession>> createSessionPair()
{
    QByteArray pickledAccountA("eOBXIKivUT6YYowRH031BNv7zNmzqM5B7CpXdyeaPvala5mt7/OeqrG1qVA7vA1SYloFyvJPIy0QNkD3j1HiPl5vtZHN53rtfZ9exXDok03zjmssqn4IJsqcA7Fbo1FZeKafG0NFcWwCPTdmcV7REqxjqGm3I4K8MQFa45AdTGSUu2C12cWeOcbSMlcINiMral+Uyah1sgPmLJ18h1qcnskXUXQvpffZ5DiUw1Iz5zxnwOQF1GVyowPJD7Zdugvj75RQnDxAn6CzyvrY2k2CuedwqDC3fIXM2xdUNWttW4nC2g4InpBhCVvNwhZYxlUb5BUEjmPI2AB3dAL5ry6o9MFncmbN6x5x");
    QByteArray pickledAccountB("eModTvoFi9oOIkax4j4nuxw9Tcl/J8mOmUctUWI68Q89HSaaPTqR+tdlKQ85v2GOs5NlZCp7EuycypN9GQ4fFbHUCrS7nspa3GFBWsR8PnM8+wez5PWmfFZLg3drOvT0jbMjpDx0MjGYClHBqcrEpKx9oFaIRGBaX6HXzT4lRaWSJkXxuX92q8iGNrLn96PuAWFNcD+2JXpPcNFntslwLUNgqzpZ04aIFYwL80GmzyOgq3Bz1GO6u3TgCQEAmTIYN2QkO0MQeuSfe7UoMumhlAJ6R8GPcdSSPtmXNk4tdyzzlgpVq1hm7ZLKto+g8/5Aq3PvnvA8wCqno2+Pi1duK1pZFTIlActr");
    auto accountA = QOlmAccount("accountA:foo.com", "Device1UserA");
    accountA.unpickle(pickledAccountA, Unencrypted{});
    auto accountB = QOlmAccount("accountB:foo.com", "Device1UserB");
    accountB.unpickle(pickledAccountB, Unencrypted{});

    const QByteArray identityKeyA("qIEr3TWcJQt4CP8QoKKJcCaukByIOpgh6erBkhLEa2o");
    const QByteArray oneTimeKeyA("WzsbsjD85iB1R32iWxfJdwkgmdz29ClMbJSJziECYwk");
    const QByteArray identityKeyB("q/YhJtog/5VHCAS9rM9uUf6AaFk1yPe4GYuyUOXyQCg");
    const QByteArray oneTimeKeyB("oWvzryma+B2onYjo3hM6A3Mgo/Yepm8HvgSvwZMTnjQ");
    auto outbound = std::get<std::unique_ptr<QOlmSession>>(accountA
        .createOutboundSession(identityKeyB, oneTimeKeyB));

    const auto preKey = std::get<Message>(outbound->encrypt("")); // Payload does not matter for PreKey

    if (preKey.type() != Message::General) {
        throw "Wrong first message type received, can't create session";
    }
    auto inbound = std::get<std::unique_ptr<QOlmSession>>(accountB.createInboundSession(preKey));
    return std::make_pair<std::unique_ptr<QOlmSession>, std::unique_ptr<QOlmSession>>(std::move(inbound), std::move(outbound));
}
