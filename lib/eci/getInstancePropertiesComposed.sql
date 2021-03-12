WITH
/**
 * Select ALL properties and their value IDs parented to the instance's parent
 * service. 
 */
InstanceValues(PropertyID, FK_PropertyValueID)
  AS(SELECT PropertyID, FK_PropertyValueID FROM Properties
			WHERE FK_Parent_InstanceID = 1 ),

/**
 * Select ALL properties and their value IDs parented to the instance.
 */
ServiceValues(PropertyID, FK_PropertyValueID)
  AS(SELECT PropertyID, FK_PropertyValueID FROM Properties
	 WHERE FK_Parent_ServiceID = 
		(SELECT FK_Parent_ServiceID FROM Instances WHERE InstanceID = 1 )),

/**
 * Collect together the instance- and service-parented property IDs and their
 * associated property value IDs, with a column to indicate whether they came
 * from a service or an instance, and also for their layer of origin and their
 * key.
 */
AllValues
  AS(SELECT 0 as Instance, PropertyID, FK_PropertyValueID, Bundle.Layer, Val.PropertyKey
	 FROM ServiceValues
	 INNER JOIN PropertyValues Val
	   ON Val.PropertyValueID = FK_PropertyValueID
	 INNER JOIN Bundles Bundle
	   ON Bundle.BundleID = Val.FK_BundleID
	 UNION
	 SELECT 1 as Instance, PropertyID, FK_PropertyValueID, Bundle.Layer, Val.PropertyKey
	 FROM InstanceValues
	 INNER JOIN PropertyValues Val
	   ON Val.PropertyValueID = FK_PropertyValueID
	 INNER JOIN Bundles Bundle
	  ON Bundle.BundleID = Val.FK_BundleID),

/**
 * Now, for each unique property key in the AllValues temporary table, get the
 * highest of the Instance column.
 */
MaxInstance(PropertyKey, Instance)
	AS(SELECT PropertyKey, MAX(Instance)
	  FROM AllValues
	  GROUP BY PropertyKey),

/**
 * And for each property name in the MaxInstance temporary table, get its
 * highest layer for that instance level.
 */
MaxLayerForHighestInstance(PropertyKey, Instance, Layer)
	AS(SELECT AllValues.PropertyKey, MaxInstance.Instance, MAX(Layer)
		FROM AllValues
		JOIN MaxInstance ON AllValues.PropertyKey = MaxInstance.PropertyKey
		WHERE AllValues.Instance = MaxInstance.Instance
		GROUP BY AllValues.PropertyKey)

/**
 * Finally we retrieve from the AllValues temporary table the property ID
 * matching the max of Layer for the max of Instance for that property key.
 */
SELECT PropertyID FROM AllValues
JOIN MaxLayerForHighestInstance
ON MaxLayerForHighestInstance.PropertyKey = AllValues.PropertyKey
    AND MaxLayerForHighestInstance.Instance = AllValues.Instance
    AND MaxLayerForHighestInstance.Layer = AllValues.Layer;
