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
	PRIMARY KEY("InstanceID" AUTOINCREMENT),
	FOREIGN KEY("FK_Parent_ServiceID") REFERENCES "Services"("ServiceID")
);
CREATE TABLE IF NOT EXISTS "Snapshots" (
	"SnapshotID"	INTEGER NOT NULL UNIQUE,
	"Name"	TEXT NOT NULL,
	"FK_Parent_InstanceID"	INTEGER NOT NULL,
	PRIMARY KEY("SnapshotID" AUTOINCREMENT),
	FOREIGN KEY("FK_Parent_InstanceID") REFERENCES "Instances"("InstanceID")
);
CREATE TABLE IF NOT EXISTS "Bundles" (
	"BundleID"	INTEGER NOT NULL UNIQUE,
	"RefCount"	INTEGER NOT NULL DEFAULT 0,
	"Filename"	INTEGER NOT NULL,
	"MD5Sum"	INTEGER NOT NULL,
	"Layer"	INTEGER NOT NULL CHECK("Layer" = 1 OR "Layer" = 2 OR "Layer" = 3 OR "Layer" = 4),
	PRIMARY KEY("BundleID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "SnapshotProperty" (
	"FK_SnapshotID"	INTEGER NOT NULL,
	"FK_PropertyID"	INTEGER NOT NULL,
	FOREIGN KEY("FK_PropertyID") REFERENCES "Properties"("PropertyID"),
	FOREIGN KEY("FK_SnapshotID") REFERENCES "Snapshots"("SnapshotID")
);
CREATE TABLE IF NOT EXISTS "Metadata" (
	"Version"	INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS "Properties" (
	"PropertyID"	INTEGER NOT NULL UNIQUE,
	"FK_Parent_InstanceID"	INTEGER,
	"FK_Parent_ServiceID"	INTEGER,
	"FK_Parent_PropertyGroupID"	INTEGER,
	"FK_PropertyValueID"	INTEGER NOT NULL,
	PRIMARY KEY("PropertyID" AUTOINCREMENT),
	FOREIGN KEY("FK_Parent_InstanceID") REFERENCES "Instances"("InstanceID"),
	FOREIGN KEY("FK_Parent_ServiceID") REFERENCES "Services"("ServiceID"),
	FOREIGN KEY("FK_PropertyValueID") REFERENCES "PropertyValues"("PropertyValueID"),
	FOREIGN KEY("FK_Parent_PropertyGroupID") REFERENCES "PropertyGroups"("PropertyGroupID")
);
CREATE TABLE IF NOT EXISTS "PropertyValues" (
	"PropertyValueID"	INTEGER NOT NULL UNIQUE,
	"FK_BundleID"	INTEGER,
	"Type"	TEXT NOT NULL CHECK("Type" = 'String' OR "Type" = 'Page'),
	"PropertyKey"	TEXT NOT NULL,
	"StringValue"	TEXT,
	"FK_PageValue_PropertyGroupID"	INTEGER,
	PRIMARY KEY("PropertyValueID" AUTOINCREMENT),
	FOREIGN KEY("FK_PageValue_PropertyGroupID") REFERENCES "PropertyGroups"("PropertyGroupID"),
	FOREIGN KEY("FK_BundleID") REFERENCES "Bundles"("BundleID")
);
CREATE TABLE IF NOT EXISTS "PropertyGroups" (
	"PropertyGroupID"	INTEGER NOT NULL UNIQUE,
	"Name"				STRING NOT NULL,
	"RefCount"	INTEGER NOT NULL DEFAULT 0,
	"FK_Parent_ServiceID"	INTEGER,
	"FK_Parent_PropertyGroupID"	INTEGER,
	PRIMARY KEY("PropertyGroupID" AUTOINCREMENT),
	FOREIGN KEY("FK_Parent_PropertyGroupID") REFERENCES "PropertyGroups"("PropertyGroupID"),
	FOREIGN KEY("FK_Parent_ServiceID") REFERENCES "Services"("ServiceID")
);
COMMIT;
