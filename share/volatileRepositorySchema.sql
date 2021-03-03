BEGIN TRANSACTION;
CREATE TABLE "LiveInstances" (
	"InstanceID"	INTEGER NOT NULL UNIQUE,
	"Name"	TEXT NOT NULL,
	"FK_Parent_ServiceID"	INTEGER NOT NULL,
	FOREIGN KEY("FK_Parent_ServiceID") REFERENCES "Services"("ServiceID"),
	PRIMARY KEY("InstanceID" AUTOINCREMENT)
);
CREATE TABLE "Metadata" (
	"Version"	INTEGER NOT NULL
);
COMMIT;
