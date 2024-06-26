import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import WidgetGroup from './WidgetGroup'; 
import MediaController from "./MediaController";
import SceneChooser from "./SceneChooser";

function ControlPanel( { dimensions, panelSize } ) {

  const [ panelJSON,     setPanelJSON ]   = useState( [] );
  const [ activeGroups,  setActiveGroups] = useState( [] );

  // Callback function to handle state changes in widget groups
  const handleWidgetGroupChange = () => {
    // Logic to update the control panel's state or perform other actions
    const active = panelJSON.filter(group => 
      window.module.is_widget_group_active(group.name)
    );
    setActiveGroups(active);
  };

  const setupPanel = () => {
    const panelJSONString = window.module.get_panel_JSON();
  
    try {
      const parsedJSON = JSON.parse(panelJSONString);
      setPanelJSON(parsedJSON);
    } catch (error) {
      console.error("Error parsing panel JSON:", error);
    }
  };

  const clearPanel = () => {
    setPanelJSON([]);
    setActiveGroups([]);
  }

  useEffect(() => {
    if (window.module) {
        setupPanel();
        window.module.set_scene_callback( setupPanel );
    } else {
        // Poll for the Module to be ready
        const intervalId = setInterval(() => {
        if (window.module) {
            setupPanel();
            window.module.set_scene_callback( setupPanel );
            clearInterval(intervalId);
        }
      }, 100); // Check every 100ms

      return () => clearInterval(intervalId);
    }
  }, [] );

  useEffect(() => {
    handleWidgetGroupChange();
  }, [ panelJSON ] );

  // Create list of widget groups from panelJSON
  // (below MediaController)         

  return (
    <Paper 
      elevation={3} 
      sx={{ 
        minWidth: dimensions.width,
        minHeight: dimensions.height,
        display: 'flex',
        flexDirection: 'row',
        flexWrap: 'wrap',
        flexGrow: 1,
        alignItems: 'flex-start',
        alignSelf: 'stretch',
        overflow: 'auto', // Enable scrolling
      }}
    >
      <div 
        style={{ 
          display: 'flex',
          flexDirection: 'column',
          flexWrap: 'wrap',
          height: dimensions.height, // Restrict height to container's height
        }}
      >
        < MediaController panelSize={panelSize} />
        < SceneChooser width = { panelSize } onChange = { clearPanel } />
        {activeGroups.map((group) => (
          // Passing handleWidgetGroupChange callback to each WidgetGroup component
          <WidgetGroup 
            key={group.name} 
            panelSize={panelSize} 
            json={group} 
            onChange={handleWidgetGroupChange} 
          />
        ))}
      </div>
    </Paper>
  );
}

export default ControlPanel; 
