set balancer(DEFAULT_NODES_GROUP) [ list LOCAL GRT TST ]

set LOCAL(HOST) localhost
set LOCAL(PORT) 8004

set GRT(HOST) test.sirena-travel.ru
set GRT(PORT) 8006


set balancer(DEDICATED_NODES_GROUPS) [ list ROBOTS ]
set ROBOTS(NODES) [ list TST ]
set ROBOTS(CLIENT_IDS) [ list 0 113 114 117 65535 ]
#set ROBOTS(QUEUE_SIZE_LIMIT) 16

set TST(HOST) test.sirena-travel.ru
set TST(PORT) 8007
