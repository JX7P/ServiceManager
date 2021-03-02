BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "Services" (
	"ServiceID"	INTEGER NOT NULL UNIQUE,
	"Name"	TEXT,
	PRIMARY KEY("ServiceID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "Instances" (
	"InstanceID"	INTEGER NOT NULL UNIQUE,
	"Name"	TEXT NOT NULL,
	"FK_Parent_ServiceID"	INTEGER NOT NULL,
	FOREIGN KEY("FK_Parent_ServiceID") REFERENCES "Services"("ServiceID"),
	PRIMARY KEY("InstanceID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "Snapshots" (
	"SnapshotID"	INTEGER NOT NULL UNIQUE,
	"Name"	TEXT NOT NULL,
	"FK_Parent_InstanceID"	INTEGER NOT NULL,
	FOREIGN KEY("FK_Parent_InstanceID") REFERENCES "Instances"("InstanceID"),
	PRIMARY KEY("SnapshotID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "Bundles" (
	"BundleID"	INTEGER NOT NULL UNIQUE,
	"Filename"	INTEGER NOT NULL,
	"MD5Sum"	INTEGER NOT NULL,
	"Layer"	INTEGER NOT NULL CHECK("Layer" = 1 OR "Layer" = 2 OR "Layer" = 3 OR "Layer" = 4),
	PRIMARY KEY("BundleID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "SnapshotProperty" (
	"FK_SnapshotID"	INTEGER NOT NULL,
	"FK_PropertyID"	INTEGER NOT NULL,
	FOREIGN KEY("FK_SnapshotID") REFERENCES "Snapshots"("SnapshotID"),
	FOREIGN KEY("FK_PropertyID") REFERENCES "Properties"("PropertyID")
);
CREATE TABLE IF NOT EXISTS "Properties" (
	"PropertyID"	INTEGER NOT NULL UNIQUE,
	"FK_Parent_InstanceID"	INTEGER,
	"FK_Parent_ServiceID"	INTEGER,
	"FK_Parent_PropertyID"	INTEGER,
	"FK_PropertyValueID"	INTEGER NOT NULL,
	FOREIGN KEY("FK_PropertyValueID") REFERENCES "PropertyValues"("PropertyValueID"),
	FOREIGN KEY("FK_Parent_InstanceID") REFERENCES "Instances"("InstanceID"),
	FOREIGN KEY("FK_Parent_ServiceID") REFERENCES "Services"("ServiceID"),
	FOREIGN KEY("FK_Parent_PropertyID") REFERENCES "Properties"("PropertyID"),
	PRIMARY KEY("PropertyID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "PropertyValues" (
	"PropertyValueID"	INTEGER NOT NULL UNIQUE,
	"FK_BundleID"	INTEGER,
	"Type"	TEXT NOT NULL CHECK("Type" = "String" OR "Type" = "Page"),
	"PropertyKey"	TEXT NOT NULL,
	"StringValue"	TEXT,
	FOREIGN KEY("FK_BundleID") REFERENCES "Bundles"("BundleID"),
	PRIMARY KEY("PropertyValueID")
);
COMMIT;
