package org.jiggawatt.squarezen;

import android.app.ActionBar;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class PlaybackFragment extends Fragment implements ActionBar.TabListener {

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) { 
    	return inflater.inflate(R.layout.activity_main, container, false); 
    }
    
    /**** TabListener ****/
    
    @Override
    public void onTabSelected(ActionBar.Tab tab,
        FragmentTransaction fragmentTransaction) {
    	// ToDo: implement    	
    }
    
    @Override
    public void onTabUnselected(ActionBar.Tab tab,
        FragmentTransaction fragmentTransaction) {
    	// ToDo: implement    	
    }

    @Override
    public void onTabReselected(ActionBar.Tab tab,
        FragmentTransaction fragmentTransaction) {
    	// ToDo: implement    	
    }
    
    /********/	
}
